#pragma once

#include <JuceHeader.h>

#include "OliversCppHeader.h"

#include "Oscillator.h"
#include "LadderFilter.h"

//------------------------------
//~ ojf: constants

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

   StereoBuffer harshFilterInput;
   LadderFilter harshFilter_l;
   LadderFilter harshFilter_r;

   StereoBuffer softFilterInput;
   LadderFilter softFilter_l;
   LadderFilter softFilter_r;

   f32 rampSamples = 0;
};

void init(PluginContext *context, f32 sampleRate, usize samplesPerBlock);
void cleanup(PluginContext *context);

void processSamples(PluginContext *context, StereoBuffer *buffer);

//- ojf: called on a separate thread from process samples!
void drawGraphics(PluginContext *context, JuceContext *juce_context);
