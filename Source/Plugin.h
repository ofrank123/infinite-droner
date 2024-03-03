#pragma once

#include <JuceHeader.h>

#include "OliversCppHeader.h"
#include <vector>

//------------------------------
//~ ojf: constants

constexpr f64 PI = 3.1415926535897932384626433832795028841971693993751;
constexpr f64 TWO_PI = PI * 2;
const f32 rampTime = 10;

//------------------------------
//~ ojf: juce

struct JuceContext {
   juce::Graphics *graphics;
};

//------------------------------
//~ ojf: graphics
struct Point {
   f32 x;
   f32 y;
};

//------------------------------
//~ ojf: oscillators

enum OscillatorType {
   OSC_SINE = 0,
   OSC_SAW,
   OSC_SQUARE,
   OSC_TRIANGLE,
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

struct Voice {
   f32 volume;
   FilterType filterType;

   Oscillator oscillator;
};

struct LFO {
   f32 current_sample = 0;

   Oscillator osc;
};

//------------------------------
//~ ojf: ladder filter

//- ojf: i'm building this on linux under clang, but it should compile
// fine in xcode because that also uses clang. if you're trying to compile
// this on vc++ i'm not sure....
typedef __attribute__((ext_vector_type(4))) f32 vector_f32_4;

struct LadderFilter {
    f32 res;
    f32 cutoff;
    f32 gain;
    f32 timestep;

    Oscillator cutoff_lfo;
    f32 cutoff_mod_base_freq;
    f32 cutoff_mod_depth;
    Oscillator meta_cutoff_lfo;
    f32 meta_cutoff_mod_depth;

    f32 sample = 0;
    f32 prev_sample = 0;
    vector_f32_4 state = {0, 0, 0, 0};
};

//------------------------------
//~ ojf: general

struct PluginContext {
   f32 sampleRate;
   f32 timeStep;
   usize samplesPerBlock;

   // TODO(ojf): Make this a queue or something...
   std::mutex samplesLock;
   Slice<f32>blockSamples;

   //- ojf: government mandated std::vector usage
   // (although in my heart it should just be a flat buffer)
   std::vector<Voice> voices;

   LadderFilter harshFilter_l;
   LadderFilter harshFilter_r;
   LadderFilter softFilter_l;
   LadderFilter softFilter_r;

   f32 rampSamples = 0;
};

struct StereoBuffer {
   Slice<f32> leftBuffer;
   Slice<f32> rightBuffer;
};

void init(PluginContext *context, f32 sampleRate, usize samplesPerBlock);
void cleanup(PluginContext *context);

void processSamples(PluginContext *context, StereoBuffer *buffer);

//- ojf: called on a separate thread from process samples!
void drawGraphics(PluginContext *context, JuceContext *juce_context);
