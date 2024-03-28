// Copyright (c) 2024 Oliver Frank
// Licensed under the GNU Public License (https://www.gnu.org/licenses/)

#include "OliversCppHeader.h"

#include "Lfo.h"

//- ojf: i'm building this on linux under clang, but it should compile
// fine in xcode because that also uses clang. if you're trying to compile
// this on vc++ i'm not sure....
typedef __attribute__ ((ext_vector_type (4))) f32 vector_f32_4;

/**
 * classic moog-style lowpass ladder filter
 */
struct LadderFilter
{
    f32 res; // resonance
    f32 cutoff; // cutoff frequency
    f32 gain; // input gain
    f32 output_gain; // output gain
    f32 timestep;

    Lfo cutoffLfo; // lfo to control cutoff
    Lfo metaCutoffLfo; // lfo to control cutoff lfo frequency

    f32 prevSample = 0; // previous output
    vector_f32_4 state = { 0, 0, 0, 0 }; // current system state
};

/**
 * fill a mono input with samples from the given oscillator
 * @param ladder filter to process
 * @param input buffer
 * @param output buffer
 */
void processLadderFilterSamples (LadderFilter* filter, Buffer input, Buffer output);
