// Copyright (c) 2024 Oliver Frank
// Licensed under the GNU Public License (https://www.gnu.org/licenses/)

#pragma once

#include "OliversCppHeader.h"

/**
 * oscillator waveforms
 */
enum OscillatorType
{
    OSC_SINE = 0,
    OSC_SAW,
    OSC_SQUARE,
    OSC_TRIANGLE,
    OSC_NOISE,
};

/**
 * filter routing
 */
enum FilterType
{
    FILT_NONE = 0,
    FILT_HARSH,
    FILT_SOFT,
};

/**
 * main oscillator
 */
struct Oscillator
{
    OscillatorType type; // waveform
    f32 sampleRate = 0; // sample rate
    f32 phase = 0; // current phase
    usize octave = 0; // octave of wavetable to index into
    f32 frequency; // base oscillator frequency
};

/**
 * fill a mono output buffer with samples from the oscillator.
 * 
 * @param oscillator to pull samples from
 * @param mono output buffer
 * @param enable frequency modulation
 * @param frequency modulation samples
 * @param enable amplitude modulation
 * @param amplitude modulation samples
 * @param enables overwriting of output buffer, otherwise accumulate
 * @param base amplitude of outputted signal
 */
void nextOscillatorSamplesMono (
    Oscillator* osc,
    Buffer output,
    bool useFreqMod,
    Buffer frequencyMod,
    bool useAmpMod,
    Buffer amplitudeMod,
    bool overwrite,
    f32 amplitude);

/**
 * fill a stereo output buffer with samples from the given oscillator.
 * 
 * @param oscillator to pull samples from
 * @param stereo output buffer
 * @param enable frequency modulation
 * @param frequency modulation samples
 * @param enable amplitude modulation
 * @param amplitude modulation samples
 * @param enables overwriting of output buffer, otherwise accumulate
 * @param base amplitude of outputted signal
 */
void nextOscillatorSamples (
    Oscillator* osc,
    StereoBuffer output,
    bool useFreqMod,
    Buffer frequencyMod,
    bool useAmpMod,
    Buffer amplitudeMod,
    bool overwrite,
    f32 amplitude);

/**
 * create an instance of the oscillator class with a given frequency
 * 
 * @param waveform of oscillator
 * @param sampling rate
 * @param base oscillator frequency
 */
Oscillator createOscillator (
    OscillatorType type,
    f32 sampleRate,
    f32 frequency);
