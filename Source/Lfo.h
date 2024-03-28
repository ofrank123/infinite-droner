// Copyright (c) 2024 Oliver Frank
// Licensed under the GNU Public License (https://www.gnu.org/licenses/)

#pragma once

#include "OliversCppHeader.h"
#include "Oscillator.h"

/**
 * low frequency oscillator to modulate parameters
 */
struct Lfo
{
    Buffer mod; // modulation samples
    Oscillator osc; // oscillator
    f32 depth; // modulation depth
};

/**
 * create an lfo
 *
 * @param waveform of lfo
 * @param sampling rate
 * @param size of block
 * @param frequency of lfo
 * @param modulation depth of lfo
 */
Lfo createLfo (
    OscillatorType type,
    f32 sampleRate,
    usize blockSize,
    f32 frequency,
    f32 depth);
