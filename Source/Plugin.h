// Copyright (c) 2024 Oliver Frank
// Licensed under the GNU Public License (https://www.gnu.org/licenses/)

#pragma once

#include <JuceHeader.h>

#include "OliversCppHeader.h"

#include "LadderFilter.h"
#include "Voice.h"

//- ojf: this is the real main entrypoint for the plugin.  i have mostly
// cordoned off my code from the juce boilerplate code, as i find a more
// c-style approach more comprehensible, using structs (which in c++ are
// just classes with public as the default accessor) and functions.
// it also has the benefit of dramatically reducing compile times, as
// all of the juce headers don't need to be recompiled with each of these
// translation units.

//------------------------------
//~ ojf: constants

const f32 rampTime = 20;

/**
 * plugin state.  stores all information for the main plugin processing
 */
struct PluginContext
{
    f32 sampleRate; // sampling rate
    usize samplesPerBlock; // block size

    //- ojf: government mandated std::vector usage
    std::vector<Voice> voices; // synth voices

    StereoBuffer harshFilterInput; // input buffer for harsh filter
    LadderFilter harshFilter_l; // left harsh filter
    LadderFilter harshFilter_r; // right harsh filter

    StereoBuffer softFilterInput; // input buffer for soft filter
    LadderFilter softFilter_l; // left soft filter
    LadderFilter softFilter_r; // right soft filter

    f32 rampSamples = 0; // samples since start of playback
};

/**
 * initialize plugin state.  to be called from the juce PluginProcessor class.
 *
 * @param context to initialize
 * @param sampling rate
 * @param samples per block
 */
void init (PluginContext* context, f32 sampleRate, usize samplesPerBlock);

/**
 * deallocate resouces in plugin state.  to be called from the juce PluginProcessor class.
 *
 * @param plugin state
 */
void cleanup (PluginContext* context);

/**
 * main dsp loop for the plugin. to be called from the juce PluginProcessor class.
 * 
 * @param plugin state
 * @param output buffer
 */
void processSamples (PluginContext* context, StereoBuffer* buffer);
