// Copyright (c) 2024 Oliver Frank
// Licensed under the GNU Public License (https://www.gnu.org/licenses/)

#pragma once

#include "Lfo.h"
#include "OliversCppHeader.h"
#include "Oscillator.h"

/**
 * main voice
 */
struct Voice
{
    f32 volume;
    f32 pan = 0.5;
    FilterType filterType;

    Oscillator oscillator;

    // frequency modulation lfo frequency modulation
    bool enableMetaFrequencyLfo;
    Lfo metaFrequencyLfo;

    // frequency modulation lfo
    bool enableFrequencyLfo;
    Lfo frequencyLfo;

    // amplitude modulation lfo frequency modulation
    bool enableMetaAmplitudeLfo;
    Lfo metaAmplitudeLfo;

    // amplitude modulation lfo
    bool enableAmplitudeLfo;
    Lfo amplitudeLfo;
};

/**
 * get the next samples from a given voice
 * @param voice to process
 * @param output buffer
 * @param enable buffer overwrite, otherwise accumulate
 */
void nextVoiceSamples (Voice* voice, StereoBuffer output, bool overwrite);
