// Copyright (c) 2024 Oliver Frank
// Licensed under the GNU Public License (https://www.gnu.org/licenses/)

#include "LadderFilter.h"

//- ojf: simulation accuracy parameter
const f32 eps = 1e-5;

//- ojf: i appreciate that this function is a little dense, and i've tried
// to comment it as best as possible. it is a nonlinear time-domain simulation of
// the classic moog ladder filter circuit. i derived the simulation
// from the system equations given in the brief for assignment 3 of pbmmi, using
// trapezoidal integration, which i initially implemented in matlab for that class.
// as the trapezoidal integration yields an implicit step function, some sort of
// numerical root finding algorithm has to be employed.  in this case, i opted for
// newton's method due to its robustness and simplicity.
//
// for this assignment, i've used clang's simd intrinsics to vectorize the code. the
// technique makes use of the fact that the jacobian is a sparse matrix, so that
// full matrix inversion or some numerical linear system solver is not required.
// the (really cool) vectorization approach is taken from this thread:
// https://www.kvraudio.com/forum/viewtopic.php?t=456207
//
// n.b. this could fairly easily be vectorized further, by combining the 4 different
// filters present in this app into one 16-lane vector unit.  however, i did not
// think this was necessary for this plugin, particularly since avx-512 support is
// still not particularly widespread.  the current code will run fine on any processor
// with sse support, which is broadly supported.

/**
 * INTERNAL simulate a single sample being processed by the filter
 * @param filter
 * @param sample to process
 * @param cutoff frequency modulation
 */
internal inline f32 processLadderFilterSample (
    LadderFilter* filter,
    f32 _sample,
    f32 cutoffMod)
{
    //- ojf: angular cutoff
    const f32 omega = (filter->cutoff + cutoffMod) * TWO_PI;
    const f32 sample = _sample * filter->gain;

    vector_f32_4 guess;
    vector_f32_4 nextGuess = filter->state;

    //- ojf: previous update function
    vector_f32_4 prev_f = omega * vector_f32_4{ -tanhf (filter->state[0]) - tanhf (4 * filter->res * filter->state[3] + filter->prevSample), -tanhf (filter->state[1]) + tanhf (filter->state[0]), -tanhf (filter->state[2]) + tanhf (filter->state[1]), -tanhf (filter->state[3]) + tanhf (filter->state[3]) };

    u8 iters = 0;

    //- ojf: newton-raphson root finding
    do
    {
        guess = nextGuess;

        //- ojf: expensive, so we cache
        vector_f32_4 guess_tanh = {
            tanhf (guess[0]),
            tanhf (guess[1]),
            tanhf (guess[2]),
            tanhf (guess[3]),
        };

        //- ojf: previous update function
        vector_f32_4 f = omega * vector_f32_4{
            -guess_tanh[0] - tanhf (4 * filter->res * guess[3] + sample),
            -guess_tanh[1] + guess_tanh[0],
            -guess_tanh[2] + guess_tanh[1],
            -guess_tanh[3] + guess_tanh[2],
        };

        //- ojf: residual
        const vector_f32_4 F = guess - filter->state - (filter->timestep / 2) * (f + prev_f);

        //- ojf: expensive, so we cache (sech^2(\omega) = 1 - tanh^2(\omega))
        const vector_f32_4 guess_sech2 = 1 - (guess_tanh * guess_tanh);

        //- ojf: jacobian calculation

        const f32 a = filter->timestep * omega / 2;
        const vector_f32_4 X = 1 + a * guess_sech2;

        const f32 Y0_tanh = 4 * filter->res * guess[3] + sample;
        const vector_f32_4 Y = -1 * vector_f32_4{
            2 * filter->timestep * omega * filter->res * (1 - Y0_tanh * Y0_tanh),
            -a * guess_sech2[0],
            -a * guess_sech2[1],
            -a * guess_sech2[2],
        };

        //- ojf: compute newton step delta
        // this is the bit from the kvr audio thread linked above
        const vector_f32_4 t1 =
            __builtin_shufflevector (F, F, 0, 1, 2, 3) * __builtin_shufflevector (X, X, 1, 0, 0, 0) * __builtin_shufflevector (X, X, 2, 2, 1, 1) * __builtin_shufflevector (X, X, 3, 3, 3, 2);
        const vector_f32_4 t2 =
            __builtin_shufflevector (F, F, 3, 0, 1, 2) * __builtin_shufflevector (Y, Y, 0, 1, 2, 3) * __builtin_shufflevector (X, X, 1, 2, 0, 0) * __builtin_shufflevector (X, X, 2, 3, 3, 1);
        const vector_f32_4 t3 =
            __builtin_shufflevector (F, F, 2, 3, 0, 1) * __builtin_shufflevector (Y, Y, 0, 0, 1, 2) * __builtin_shufflevector (Y, Y, 3, 1, 2, 3) * __builtin_shufflevector (X, X, 1, 2, 3, 0);
        const vector_f32_4 t4 =
            __builtin_shufflevector (F, F, 1, 2, 3, 0) * __builtin_shufflevector (Y, Y, 0, 0, 0, 1) * __builtin_shufflevector (Y, Y, 2, 1, 1, 2) * __builtin_shufflevector (Y, Y, 3, 3, 2, 3);

        //- ojf: jacobian determinant
        const float det = (X[0] * X[1] * X[2] * X[3]) - (Y[0] * Y[1] * Y[2] * Y[3]);
        const vector_f32_4 delta = (t1 + t2 + t3 + t4) / det;

        //- ojf: calculate new guess
        nextGuess = guess - delta;

        iters += 1;

        //- ojf: check if the current guess is good enough, or if we've maxed
        // out the allowed iterations
    } while ((fabs (nextGuess[0] - guess[0])) + (fabs (nextGuess[1] - guess[1])) + (fabs (nextGuess[2] - guess[2])) + (fabs (nextGuess[3] - guess[3])) > eps && iters < 10);

    //- ojf: update state
    filter->state = nextGuess;

    return filter->state[3];
}

void processLadderFilterSamples (LadderFilter* filter, Buffer input, Buffer output)
{
    //- ojf: calculate meta-lfo modulation samples
    nextOscillatorSamplesMono (
        &filter->metaCutoffLfo.osc,
        filter->metaCutoffLfo.mod,
        false,
        {},
        false,
        {},
        true,
        filter->metaCutoffLfo.depth);

    //- ojf: calculate cutoff modulation samples
    nextOscillatorSamplesMono (
        &filter->cutoffLfo.osc,
        filter->cutoffLfo.mod,
        true,
        filter->metaCutoffLfo.mod,
        false,
        {},
        true,
        filter->cutoffLfo.depth);

    //- ojf: process samples
    for (int i = 0; i < output.len; i++)
    {
        output[i] += processLadderFilterSample (
            filter,
            input[i],
            filter->cutoffLfo.mod[i]);
    }
}
