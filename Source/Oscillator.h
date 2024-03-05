#pragma once

#include "OliversCppHeader.h"

enum OscillatorType {
   OSC_SINE = 0,
   OSC_SAW,
   OSC_SQUARE,
   OSC_TRIANGLE,
   OSC_NOISE,
};

enum FilterType {
   FILT_NONE = 0,
   FILT_HARSH,
   FILT_SOFT,
};

struct Oscillator {
   OscillatorType type;
   f32 sampleRate = 0;

   f32 phase = 0;
   f32 phaseDelta = 0;
   usize octave = 0;

   f32 pulseWidth = 0.5;
   f32 frequency;
};

struct Lfo {
   Slice<f32> mod;
   Oscillator osc;
   f32 depth;
};

struct Voice {
   f32 volume;
   f32 pan = 0.5;
   FilterType filterType;

   Oscillator oscillator;

   bool enableMetaFrequencyLfo;
   Lfo metaFrequencyLfo;
   bool enableFrequencyLfo;
   Lfo frequencyLfo;

   bool enableMetaAmplitudeLfo;
   Lfo metaAmplitudeLfo;
   bool enableAmplitudeLfo;
   Lfo amplitudeLfo;
};

void nextOscillatorSamplesMono(
    Oscillator *osc,
    Slice<f32> output,
    bool useFreqMod,
    Slice<f32> frequencyMod,
    bool useAmpMod,
    Slice<f32> amplitudeMod,
    bool overwrite,
    f32 amplitude
);

void nextOscillatorSamples(
    Oscillator *osc,
    StereoBuffer output,
    bool useFreqMod,
    Slice<f32> frequencyMod,
    bool useAmpMod,
    Slice<f32> amplitudeMod,
    bool overwrite,
    f32 amplitude
);

Oscillator createOscillator(OscillatorType type, f32 sampleRate, f32 frequency);
void setOscillatorFrequency(Oscillator *osc, f32 frequency);

Lfo createLfo(OscillatorType type, 
              f32 sampleRate, 
              usize blockSize, 
              f32 frequency, 
              f32 depth);

void nextVoiceSamples(Voice *voice, StereoBuffer output, bool overwrite);
