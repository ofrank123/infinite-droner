#include "Plugin.h"

#include <cassert>
#include <cmath>

#include "./tables_N2048_f40_o9.h"
const usize wavetable_samples = 2048;
const f32 wavetable_f0 = 40;
const f32 wavetable_octaves = 9;

//------------------------------
//~ ojf: memory arenas

internal MArena createArena(usize size) {
    u8 *mem = (u8*) calloc(sizeof(u8), size);
    return {
        .ptr = mem,
        .bytesAllocated = size,
        .head = mem,
    };
}

internal inline u8 *arenaAlloc(MArena *arena, usize bytes) {
    assert((arena->head - arena->ptr) + bytes <= arena->bytesAllocated);
    u8 *ret = arena->head;
    arena->head += bytes;

    return ret;
}

internal inline void freeArena(MArena *arena) {
    free(arena->ptr);
}


//------------------------------
//~ ojf: qwik mafs

//- ojf: slow dft
internal void dft(Slice<f32> x, Slice<complex32> X) {
    assert(x.len == X.len);

    const f32 N = x.len;

    const complex32 j(0.0, 1.0);
    for (int k=0; k < x.len; k++) {
        X[k] = complex32(0, 0);
        for (int n=0; n < x.len; n++) {
            X[k] += x[n] * std::exp(-j*2.0f*(f32)PI*(f32)k*(f32)n/N);
        }
    }
}

//- ojf: a simple little fft (cooley-tukey '65).
// TODO(ojf): could probably vectorize...
internal void fft_arena(MArena *arena, Slice<f32> x, Slice<complex32> X) {
    assert((x.len & (x.len - 1)) == 0);
    assert(x.len != 0);
    assert(x.len == X.len);
    const usize N = x.len;

    const complex32 j(0.0, 1.0);
    if (N > 16) {
        Slice<f32> x_even = {
            .ptr = (f32*) arenaAlloc(arena, N/2 * sizeof(f32)),
            .len = N/2,
        };
        for (int i = 0; i < N/2; i++) {
            x_even[i]=x[i*2];
        }

        Slice<complex32> X_even = {
            .ptr = (complex32*) arenaAlloc(arena, N/2 * sizeof(complex32)),
            .len = N / 2,
        };
        fft_arena(arena, x_even, X_even);

        Slice<f32> x_odd = {
            .ptr = (f32*) arenaAlloc(arena, N/2 * sizeof(f32)),
            .len = N/2,
        };
        for (int i = 0; i < N/2; i++) {
            x_odd[i]=x[i*2 + 1];
        }

        Slice<complex32> X_odd = {
            .ptr = (complex32*) arenaAlloc(arena, N/2 * sizeof(complex32)),
            .len = N / 2,
        };
        fft_arena(arena, x_odd, X_odd);

        for (usize k = 0; k < N/2; k++) {
            complex32 factor = std::exp(-j*2.0f*(f32)PI*(f32)k/(f32)N);
            X[k]       = X_even[k] + factor * X_odd[k];
            X[k + N/2] = X_even[k] - factor * X_odd[k];
        }
    } else {
        return dft(x, X);
    }
}

internal void fft(Slice<f32> x, Slice<complex32> X) {
    //- ojf: upper bound on memory needed
    MArena arena = createArena(
        sizeof(f32) * x.len * log2(x.len) +
        sizeof(complex32) * x.len * log2(x.len)
    );
    fft_arena(&arena, x, X);
    freeArena(&arena);
}

//- ojf: Compute the spectrum in dB
internal void fft_db(Slice<f32> x, Slice<f32> X) {
    Slice<complex32> Xcomplex = {
        .ptr = (complex32*) calloc(X.len, sizeof(complex32)),
        .len = X.len,
    };
    fft(x, Xcomplex);

    for (int k=0; k<X.len; k++) {
        X[k] = 20*log10(abs(Xcomplex[k]));
    }

    free(Xcomplex.ptr);
}

internal f32 sliceMax(Slice<f32> x) {
    f32 ret = 0;
    for (int i=0;i<x.len;i++) {
        ret = fmax(x[i], ret);
    }

    return ret;
}

internal usize umax(usize a, usize b) {
    return a > b ? a : b;
}

internal usize umin(usize a, usize b) {
    return a < b ? a : b;
}

//------------------------------
//~ ojf: juce graphics

internal void drawLine(JuceContext *j, Point start, Point end, u32 color) {
    juce::Line<f32> line (juce::Point<f32> (start.x, start.y),
                          juce::Point<f32> (end.x, end.y));
    j->graphics->setColour(juce::Colour(0xff000000 | color));
    j->graphics->drawLine(line, 1.0f);
}

internal void drawRect(JuceContext *j, Point pos, Point size, u32 color) {
    j->graphics->setColour(juce::Colour(0xff000000 | color));
    j->graphics->drawRect(pos.x, pos.y, size.x, size.y);
}
//------------------------------
//~ ojf: oscillators + voices

internal Oscillator initOscillator(
    OscillatorType type,
    f32 frequency,
    f32 sampleRate
) {
    return {
        .type = type,
        .phase = 0,
        .pulseWidth = 0.5,
        .frequency = frequency,
    };
}

internal f32 nextOscillatorSample(Oscillator *osc) {
    osc->phase += osc->phaseDelta;

    bool startBlep = false;
    while(osc->phase > 1) {
        startBlep = true;
        osc->phase -= 1;
    }

    f32 table_offset = osc->octave * wavetable_samples;
    f32 table_idx = table_offset + osc->phase * wavetable_samples;

    f32 sample = 0;
    switch (osc->type) {
        case OSC_SINE: {
            sample = sin(TWO_PI * osc->phase);
            break;
        }
        case OSC_SQUARE: {
            f32 table_sample_l = square_N2048_f40_o9[(usize) floor(table_idx)];
            f32 table_sample_r = square_N2048_f40_o9[(usize) ceil(table_idx)];
            sample = 0.5 * (table_sample_l + table_sample_r);
            break;
        }
        case OSC_SAW: {
            f32 table_sample_l = saw_N2048_f40_o9[(usize) floor(table_idx)];
            f32 table_sample_r = saw_N2048_f40_o9[(usize) ceil(table_idx)];
            sample = 0.5 * (table_sample_l + table_sample_r);
            break;
        }
        case OSC_TRIANGLE: {
            f32 table_sample_l = triangle_N2048_f40_o9[(usize) floor(table_idx)];
            f32 table_sample_r = triangle_N2048_f40_o9[(usize) ceil(table_idx)];
            sample = 0.5 * (table_sample_l + table_sample_r);
            break;
        }
    }

    return sample;
}


internal void setOscillatorFrequency(Oscillator *osc, f32 frequency) {
    osc->frequency = frequency;
    osc->phaseDelta = osc->frequency / osc->sampleRate;

    usize octave = 0; 
    if (frequency > wavetable_f0) {
        f32 f0 = wavetable_f0;

        for (int n=0; n <= wavetable_octaves; n++) {
            octave = n;
            if (frequency < f0*2) {
                break;
            }
            f0 *= 2;
        }
    }

    osc->octave = octave;
}

internal Oscillator createOscillator(OscillatorType type, f32 sampleRate, f32 frequency) {
    Oscillator osc = {
        .type = type,
        .sampleRate = sampleRate
    };

    setOscillatorFrequency(&osc, frequency);
    return osc;
}

internal f32 nextVoiceSample(Voice *voice) {
    return nextOscillatorSample(&voice->oscillator) * voice->volume;
}

//------------------------------
//~ ojf: ladder filter

const f32 eps = 1e-5;

//- ojf: i appreciate that this function is a little intimidating.  i've tried
// to comment it as best as possible, but at it's core, it's a nonlinear time domain
// simulation of the classic moog ladder filter circuit.  i derived the simulation
// from the system equations given in the brief for assignment 3 of pbmmi, using
// trapezoidal integration, which i initially implemented in matlab for that class.
// as the trapezoidal integration yields an implicit step function, some sort of
// numerical root finding algorithm has to be employed.  in this case, i opted for
// newton's method due to its robustness and simplicity.
//
// for this assignment, i've used clang's simd intrinsics to vectorize the code. the
// technique makes use of the fact that the jacobian is a sparse matrix, so that 
// full matrix inversion or some numerical linear system solver is not required.
// the (really cool) vectorization approach taken is taken from this thread:
// https://www.kvraudio.com/forum/viewtopic.php?t=456207
internal f32 processLadderFilter(LadderFilter* filter) {
    //- ojf: modulate the modulators -jules
    const f32 cutoff_mod_freq = filter->cutoff_mod_base_freq + 
        filter->meta_cutoff_mod_depth * nextOscillatorSample(
            &filter->meta_cutoff_lfo
        );
    setOscillatorFrequency(
        &filter->cutoff_lfo,
        cutoff_mod_freq
    );

    //- ojf: cutoff lfo
    const f32 cutoff = filter->cutoff +
        filter->cutoff_mod_depth * nextOscillatorSample(&filter->cutoff_lfo);

    //- ojf: angular cutoff
    const f32 omega = cutoff * TWO_PI;
    const f32 sample = filter->sample * filter->gain;

    vector_f32_4 guess;
    vector_f32_4 next_guess = filter->state;

    //- ojf: previous update function
    vector_f32_4 prev_f = omega * vector_f32_4 {
        -tanhf(filter->state[0])-tanhf(4*filter->res*filter->state[3]+filter->prev_sample),
        -tanhf(filter->state[1])+tanhf(filter->state[0]),
        -tanhf(filter->state[2])+tanhf(filter->state[1]),
        -tanhf(filter->state[3])+tanhf(filter->state[3])    
    };

    u8 iters = 0;

    do {
        guess = next_guess;

        //- ojf: expensive, so we cache
        vector_f32_4 guess_tanh = {
            tanhf(guess[0]),
            tanhf(guess[1]),
            tanhf(guess[2]),
            tanhf(guess[3]),
        };

        //- ojf: previous update function
        vector_f32_4 f = omega * vector_f32_4 {
            -guess_tanh[0]-tanhf(4*filter->res*guess[3]+sample),
            -guess_tanh[1]+guess_tanh[0],
            -guess_tanh[2]+guess_tanh[1],
            -guess_tanh[3]+guess_tanh[2],
        };

        //- ojf: residual
        const vector_f32_4 F = guess - filter->state - (filter->timestep / 2) * (f + prev_f);

        //- ojf: expensive, so we cache (sech^2(\omega) = 1 - tanh^2(\omega))
        const vector_f32_4 guess_sech2 = 1 - (guess_tanh * guess_tanh);

        //- ojf coefficient
        const f32 a = filter->timestep * omega / 2;
        const vector_f32_4 X = 1 + a*guess_sech2;

        const f32 Y0_tanh = 4*filter->res*guess[3]+sample;
        const vector_f32_4 Y = -1 * vector_f32_4 {
            2*filter->timestep*omega*filter->res*(1 - Y0_tanh * Y0_tanh),
            -a*guess_sech2[0],
            -a*guess_sech2[1],
            -a*guess_sech2[2],
        };

        //- ojf: compute newton step delta
        const vector_f32_4 t1 =
            __builtin_shufflevector(F,F,0,1,2,3) *
            __builtin_shufflevector(X,X,1,0,0,0) *
            __builtin_shufflevector(X,X,2,2,1,1) *
            __builtin_shufflevector(X,X,3,3,3,2);
        const vector_f32_4 t2 =
            __builtin_shufflevector(F,F,3,0,1,2) *
            __builtin_shufflevector(Y,Y,0,1,2,3) *
            __builtin_shufflevector(X,X,1,2,0,0) *
            __builtin_shufflevector(X,X,2,3,3,1);
        const vector_f32_4 t3 =
            __builtin_shufflevector(F,F,2,3,0,1) *
            __builtin_shufflevector(Y,Y,0,0,1,2) *
            __builtin_shufflevector(Y,Y,3,1,2,3) *
            __builtin_shufflevector(X,X,1,2,3,0);
        const vector_f32_4 t4 =
            __builtin_shufflevector(F,F,1,2,3,0) *
            __builtin_shufflevector(Y,Y,0,0,0,1) *
            __builtin_shufflevector(Y,Y,2,1,1,2) *
            __builtin_shufflevector(Y,Y,3,3,2,3);

        //- ojf: determinant
        const float det = (X[0] * X[1] * X[2] * X[3]) - (Y[0] * Y[1] * Y[2] * Y[3]);    
        const vector_f32_4 delta = (t1 + t2 + t3 + t4) / det;

        next_guess = guess - delta;

        iters += 1;
    } while ((fabs(next_guess[0] - guess[0])) +
             (fabs(next_guess[1] - guess[1])) + 
             (fabs(next_guess[2] - guess[2])) +
             (fabs(next_guess[3] - guess[3])) > eps && iters < 10);

    filter->state = next_guess;

    //- ojf: ready to accept next sample
    filter->sample = 0;
    return filter->state[3];
}

//------------------------------
//~ ojf: initialization + cleanup

void init(PluginContext *context, f32 sampleRate, usize samplesPerBlock) {
    context->sampleRate = sampleRate;
    context->timeStep = sampleRate;
    context->samplesPerBlock = samplesPerBlock;
    context->blockSamples = {
        .ptr = (f32 *) calloc(samplesPerBlock, sizeof(f32)),
        .len = samplesPerBlock,
    };


    const f32 master_level = 0.8;
    { //- ojf: sub
        context->voices.push_back({
            .volume = 0.2f * master_level,
            .filterType = FILT_NONE,
            .oscillator = createOscillator(
                OSC_SINE, sampleRate, 40
            ),
        });
    }

    { //- ojf: saws
        u32 voices = 3;
        for (int i = 0; i < voices; i++) {
            context->voices.push_back({
                .volume = 0.1f * master_level,
                .filterType = FILT_HARSH,
                .oscillator = createOscillator(
                    OSC_TRIANGLE, sampleRate, 50 + i * 50.2
                ),
            });
        }
    }

    //- ojf: lead voices
    { 
        context->voices.push_back({
            .volume = 0.2f * master_level,
            .filterType = FILT_SOFT,
            .oscillator = createOscillator(OSC_TRIANGLE, sampleRate, 440.33),
        });
        context->voices.push_back({
            .volume = 0.2f * master_level,
            .filterType = FILT_SOFT,
            .oscillator = createOscillator(OSC_TRIANGLE, sampleRate, 587.33),
        });
        context->voices.push_back({
            .volume = 0.2f * master_level,
            .filterType = FILT_SOFT,
            .oscillator = createOscillator(OSC_TRIANGLE, sampleRate, 659.26),
        });
    }

    // gainey resonant filter
    context->harshFilter_l = {
        .res = 1.5f,
        .cutoff = 600.0f,
        .gain = 20.0f,
        .timestep = 1 / sampleRate,
        .cutoff_lfo = createOscillator(OSC_SINE, sampleRate, 0.05f),
        .cutoff_mod_base_freq = 0.0005f,
        .cutoff_mod_depth = 600,
        .meta_cutoff_lfo = createOscillator(OSC_SINE, sampleRate, 0.0005f),
        .meta_cutoff_mod_depth = 0.09f,
    };
    context->harshFilter_r = {
        .res = 1.5f,
        .cutoff = 600.0f,
        .gain = 20.0f,
        .timestep = 1 / sampleRate,
        .cutoff_lfo = createOscillator(OSC_SINE, sampleRate, 0.004f),
        .cutoff_mod_base_freq = 0.0004f,
        .cutoff_mod_depth = 550,
        .meta_cutoff_lfo = createOscillator(OSC_SINE, sampleRate, 0.0006f),
        .meta_cutoff_mod_depth = 0.087f,
    };

    context->softFilter_l = {
        .res = 0.2f,
        .cutoff = 2000.0f,
        .gain = 1.0f,
        .timestep = 1 / sampleRate,
        .cutoff_lfo = createOscillator(OSC_SINE, sampleRate, 0.03f),
        .cutoff_mod_base_freq = 0.03f,
        .cutoff_mod_depth = 1000.0f,
        .meta_cutoff_lfo = createOscillator(OSC_SINE, sampleRate, 0.001f),
        .meta_cutoff_mod_depth = 0.02f,
    };
    context->softFilter_r = {
        .res = 0.3f,
        .cutoff = 2000.0f,
        .gain = 1.0f,
        .timestep = 1 / sampleRate,
        .cutoff_lfo = createOscillator(OSC_SINE, sampleRate, 0.0025f),
        .cutoff_mod_base_freq = 0.032f,
        .cutoff_mod_depth = 1000.0f,
        .meta_cutoff_lfo = createOscillator(OSC_SINE, sampleRate, 0.0015f),
        .meta_cutoff_mod_depth = 0.02f,
    };
}

void cleanup(PluginContext *context) {
    free(context->blockSamples.ptr);
}

//------------------------------
//~ ojf: main dsp loop

void processSamples(PluginContext *context, StereoBuffer *buffer) {
    assert(buffer->rightBuffer.len == buffer->leftBuffer.len);
    assert(buffer->rightBuffer.len == context->samplesPerBlock);

    const usize bufferLen = buffer->rightBuffer.len;

    context->samplesLock.lock();
    for (usize i = 0; i < bufferLen; i++) {
        f32 sample_l = 0;
        f32 sample_r = 0;
        for (Voice& voice : context->voices) {
            const f32 voice_sample = nextVoiceSample(&voice);
            switch (voice.filterType) {
                case FILT_NONE:
                    sample_l += voice_sample;
                    sample_r += voice_sample;
                    break;
                case FILT_HARSH:
                    context->harshFilter_l.sample += voice_sample;
                    context->harshFilter_r.sample += voice_sample;
                    break;
                case FILT_SOFT:
                    context->softFilter_l.sample += voice_sample;
                    context->softFilter_r.sample += voice_sample;
                    break;
            }
        }

        sample_l += processLadderFilter(&context->harshFilter_l);
        sample_r += processLadderFilter(&context->harshFilter_r);
        sample_l += processLadderFilter(&context->softFilter_l);
        sample_r += processLadderFilter(&context->softFilter_r);

        if (context->rampSamples < rampTime * context->sampleRate) {
            context->rampSamples += 1;
            const f32 ramp_atn = (f32) context->rampSamples / 
                (rampTime * context->sampleRate);
            sample_l *= ramp_atn;
            sample_r *= ramp_atn;
        }

        buffer->rightBuffer[i] += sample_l;
        buffer->leftBuffer[i]  += sample_r;
        context->blockSamples[i] = (sample_l + sample_r) / 2.0;
    }
    context->samplesLock.unlock();
}

//------------------------------
//~ ojf: main graphics call

void drawGraphics(PluginContext *context, JuceContext *j) {
    f32 width = 400;
    f32 height = 300;
    usize N = context->blockSamples.len;

    context->samplesLock.lock();
    {
        usize zeroPoint = 0;
        for (; zeroPoint < N; zeroPoint++) {
            if (fabs(context->blockSamples[zeroPoint]) <= 0.01) {
                break;
            }
        }

        usize end = umin(zeroPoint + N/2, N);
        usize range = end - zeroPoint;
        for (int i = zeroPoint; i < end; i++) {
            f32 startX = width * ((f32) (i - 1 - zeroPoint) / ((f32)range));
            f32 startY = 50 + 50 * context->blockSamples[i - 1];
            f32 endX = width * ((f32) (i - zeroPoint) / ((f32)range));
            f32 endY = 50 + 50 * context->blockSamples[i];
            drawLine(j, { startX, startY }, { endX, endY }, 0x0EB53B);
        }
    }

    {
        f32 specHeight = 150;
        Slice<f32> blockSamplesSpec =  {
            .ptr = (f32*) calloc(sizeof(f32), N),
            .len = N,
        };
        fft_db(context->blockSamples, blockSamplesSpec);
        f32 dbRange = 60;
        f32 maxSpecVal = sliceMax(blockSamplesSpec);
        f32 minSpecVal = maxSpecVal - dbRange;
        for (usize i = 1; i < N/2; i++) {
            f32 startX = width * ((f32) (i - 1) / ((f32)N/2));
            f32 startY = height - specHeight * 
                ((blockSamplesSpec[i - 1] - minSpecVal) / (dbRange));
            f32 endX = width * ((f32) i / ((f32)N/2));
            f32 endY = height - specHeight *
                ((blockSamplesSpec[i] - minSpecVal) / (dbRange));
            drawLine(j, { startX, startY }, { endX, endY }, 0x0EB53B);
        }
        free(blockSamplesSpec.ptr);
    }

    context->samplesLock.unlock();
}
