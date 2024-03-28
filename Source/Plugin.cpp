// Copyright (c) 2024 Oliver Frank
// Licensed under the GNU Public License (https://www.gnu.org/licenses/)

#include "Plugin.h"

#include <cassert>
#include <cstring>

//------------------------------
//~ ojf: memory

void clearSlice (Buffer slice)
{
    //- ojf: write 0 bytes
    memset (slice.ptr, 0, sizeof (f32) * slice.len);
}

void clearStereoBuffer (StereoBuffer buffer)
{
    clearSlice (buffer.leftBuffer);
    clearSlice (buffer.rightBuffer);
}

Buffer createSlice (usize len)
{
    return {
        .ptr = (f32*) calloc (len, sizeof (f32)),
        .len = len,
    };
}

StereoBuffer createStereoBuffer (usize len)
{
    return {
        .leftBuffer = createSlice (len),
        .rightBuffer = createSlice (len),
    };
}

//------------------------------
//~ ojf: initialization + cleanup

void init (PluginContext* context, f32 sampleRate, usize samplesPerBlock)
{
    context->sampleRate = sampleRate;
    context->samplesPerBlock = samplesPerBlock;

    //- ojf: create buffers
    context->harshFilterInput = createStereoBuffer (samplesPerBlock);
    context->softFilterInput = createStereoBuffer (samplesPerBlock);

    //------------------------------
    //~ ojf: voice initialization
    //
    // this is where most of the parameters for the drone are set.
    // the rest of the code was setup in a manner that allows this
    // declarative, struct based syntax, which reads off like a
    // configuration file. as everything is pretty explicit, i
    // won't be extensively commenting this section.  the values
    // were chosen by listening to the drone, and slowly tweaking
    // the values until i arrived at something i was happy with.
    // although not strictly necessary, i have used brackets to
    // separate the different logical groups of voices.

    { //- ojf: subs
        context->voices.push_back ({
            .volume = 0.1f,
            .filterType = FILT_NONE,
            .oscillator = createOscillator (
                OSC_SINE, sampleRate, 50),
            .amplitudeLfo = createLfo (
                OSC_TRIANGLE,
                sampleRate,
                samplesPerBlock,
                0.001,
                0.1),
        });
        context->voices.push_back ({
            .volume = 0.05f,
            .filterType = FILT_NONE,
            .oscillator = createOscillator (
                OSC_SINE, sampleRate, 40),
            .amplitudeLfo = createLfo (
                OSC_TRIANGLE,
                sampleRate,
                samplesPerBlock,
                0.0005,
                0.05),
        });
    }

    { //- ojf: noise
        context->voices.push_back ({
            .volume = 0.2f,
            .filterType = FILT_HARSH,
            .oscillator = createOscillator (
                OSC_NOISE, sampleRate, 40),
        });
    }

    { //- ojf: saws
        u32 voices = 3;
        for (int i = 0; i < voices; i++)
        {
            context->voices.push_back ({
                .volume = 0.1f,
                .filterType = FILT_SOFT,
                .oscillator = createOscillator (
                    OSC_SAW, sampleRate, 100 + i * 50.5),
                .enableAmplitudeLfo = true,
                .amplitudeLfo = createLfo (
                    OSC_SAW,
                    sampleRate,
                    samplesPerBlock,
                    0.001 + i * 0.001,
                    0.02),
            });
        }
    }

    //- ojf: lead voices
    {
        context->voices.push_back ({
            .volume = 0.2f,
            .filterType = FILT_SOFT,
            .oscillator = createOscillator (OSC_TRIANGLE, sampleRate, 440.33),
            .enableFrequencyLfo = true,
            .frequencyLfo = createLfo (OSC_SINE, sampleRate, samplesPerBlock, 2, 1),
        });
        context->voices.push_back ({
            .volume = 0.2f,
            .filterType = FILT_SOFT,
            .oscillator = createOscillator (OSC_TRIANGLE, sampleRate, 587.33),
            .enableMetaFrequencyLfo = true,
            .metaFrequencyLfo = createLfo (OSC_SINE, sampleRate, samplesPerBlock, 0.001, 3),
            .enableFrequencyLfo = true,
            .frequencyLfo = createLfo (OSC_SINE, sampleRate, samplesPerBlock, 0.05, 5),
            .enableAmplitudeLfo = true,
            .amplitudeLfo = createLfo (OSC_SAW, sampleRate, samplesPerBlock, 0.001, 0.4),
        });
        context->voices.push_back ({
            .volume = 0.2f,
            .filterType = FILT_SOFT,
            .oscillator = createOscillator (OSC_TRIANGLE, sampleRate, 659.26),
            .enableMetaFrequencyLfo = true,
            .metaFrequencyLfo = createLfo (OSC_SINE, sampleRate, samplesPerBlock, 0.02, 1.5),
            .enableFrequencyLfo = true,
            .frequencyLfo = createLfo (OSC_SINE, sampleRate, samplesPerBlock, 0.006, 7),
            .enableAmplitudeLfo = true,
            .amplitudeLfo = createLfo (OSC_SAW, sampleRate, samplesPerBlock, 0.003, 0.4),
        });
    }

    { //- ojf: ringing
        context->voices.push_back ({
            .volume = 0.1f,
            .filterType = FILT_NONE,
            .oscillator = createOscillator (OSC_SINE, sampleRate, 700),
            .enableMetaFrequencyLfo = true,
            .metaFrequencyLfo = createLfo (OSC_SINE, sampleRate, samplesPerBlock, 0.001, 2),
            .enableFrequencyLfo = true,
            .frequencyLfo = createLfo (OSC_SINE, sampleRate, samplesPerBlock, 2000, 100),
            .enableMetaAmplitudeLfo = true,
            .metaAmplitudeLfo = createLfo (OSC_SQUARE, sampleRate, samplesPerBlock, 0.0002, 0.2),
            .enableAmplitudeLfo = true,
            .amplitudeLfo = createLfo (OSC_SINE, sampleRate, samplesPerBlock, 0.002, 0.003),
        });
        context->voices.push_back ({
            .volume = 0.1f,
            .filterType = FILT_NONE,
            .oscillator = createOscillator (OSC_SINE, sampleRate, 666),
            .enableMetaFrequencyLfo = true,
            .metaFrequencyLfo = createLfo (OSC_SINE, sampleRate, samplesPerBlock, 0.001, 2),
            .enableFrequencyLfo = true,
            .frequencyLfo = createLfo (OSC_SINE, sampleRate, samplesPerBlock, 1200, 100),
            .enableMetaAmplitudeLfo = true,
            .metaAmplitudeLfo = createLfo (OSC_SQUARE, sampleRate, samplesPerBlock, 0.0001, 0.2),
            .enableAmplitudeLfo = true,
            .amplitudeLfo = createLfo (OSC_SINE, sampleRate, samplesPerBlock, 0.001, 0.005),
        });
        context->voices.push_back ({
            .volume = 0.1f,
            .filterType = FILT_SOFT,
            .oscillator = createOscillator (OSC_SAW, sampleRate, 1500),
            .enableFrequencyLfo = true,
            .frequencyLfo = createLfo (OSC_SINE, sampleRate, samplesPerBlock, 2, 0.02),
            .enableMetaAmplitudeLfo = true,
            .metaAmplitudeLfo = createLfo (OSC_SQUARE, sampleRate, samplesPerBlock, 0.00001, 0.2),
            .enableAmplitudeLfo = true,
            .amplitudeLfo = createLfo (OSC_SINE, sampleRate, samplesPerBlock, 0.0001, 0.005),
        });
    }

    //------------------------------
    //~ ojf: filter initialization

    {
        // gainey resonant filter
        context->harshFilter_l = {
            .res = 1.0f,
            .cutoff = 600.0f,
            .gain = 10.0f,
            .output_gain = 1.0f,
            .timestep = 1 / sampleRate,
            .cutoffLfo = createLfo (OSC_SINE, sampleRate, samplesPerBlock, 0.0004, 400),
            .metaCutoffLfo = createLfo (OSC_SINE, sampleRate, samplesPerBlock, 0.005, 0.09f),
        };
        context->harshFilter_r = {
            .res = 1.0f,
            .cutoff = 600.0f,
            .gain = 10.0f,
            .output_gain = 1.0f,
            .timestep = 1 / sampleRate,
            .cutoffLfo = createLfo (OSC_SINE, sampleRate, samplesPerBlock, 0.0005, 500),
            .metaCutoffLfo = createLfo (OSC_SINE, sampleRate, samplesPerBlock, 0.006, 0.07f),
        };

        // mellower filter
        context->softFilter_l = {
            .res = 0.2f,
            .cutoff = 2000.0f,
            .gain = 2.0f,
            .output_gain = 1.0f,
            .timestep = 1 / sampleRate,
            .cutoffLfo = createLfo (OSC_SINE, sampleRate, samplesPerBlock, 0.003, 1000),
            .metaCutoffLfo = createLfo (OSC_SINE, sampleRate, samplesPerBlock, 0.001, 0.02f),
        };
        context->softFilter_r = {
            .res = 0.3f,
            .cutoff = 2000.0f,
            .gain = 2.0f,
            .output_gain = 1.0f,
            .timestep = 1 / sampleRate,
            .cutoffLfo = createLfo (OSC_SINE, sampleRate, samplesPerBlock, 0.0025, 1000),
            .metaCutoffLfo = createLfo (OSC_SINE, sampleRate, samplesPerBlock, 0.0015, 0.02f),
        };
    }
}

void cleanup (PluginContext* context)
{
    //- ojf: free filter input buffers
    free (context->harshFilterInput.leftBuffer.ptr);
    free (context->harshFilterInput.rightBuffer.ptr);
    free (context->softFilterInput.leftBuffer.ptr);
    free (context->softFilterInput.rightBuffer.ptr);

    //- ojf: free modulation buffers
    for (Voice& voice : context->voices)
    {
        if (voice.enableMetaFrequencyLfo)
        {
            free (voice.metaFrequencyLfo.mod.ptr);
        }
        if (voice.enableFrequencyLfo)
        {
            free (voice.frequencyLfo.mod.ptr);
        }
        if (voice.enableMetaAmplitudeLfo)
        {
            free (voice.metaAmplitudeLfo.mod.ptr);
        }
        if (voice.enableAmplitudeLfo)
        {
            free (voice.amplitudeLfo.mod.ptr);
        }
    }

    free (context->harshFilter_l.metaCutoffLfo.mod.ptr);
    free (context->harshFilter_l.cutoffLfo.mod.ptr);
    free (context->harshFilter_r.metaCutoffLfo.mod.ptr);
    free (context->harshFilter_r.cutoffLfo.mod.ptr);
    free (context->softFilter_l.metaCutoffLfo.mod.ptr);
    free (context->softFilter_l.cutoffLfo.mod.ptr);
    free (context->softFilter_r.metaCutoffLfo.mod.ptr);
    free (context->softFilter_r.cutoffLfo.mod.ptr);
}

//------------------------------
//~ ojf: main dsp loop

void processSamples (PluginContext* context, StereoBuffer* buffer)
{
    assert (buffer->rightBuffer.len == buffer->leftBuffer.len);
    assert (buffer->rightBuffer.len == context->samplesPerBlock);

    const usize bufferLen = buffer->rightBuffer.len;

    //- ojf: keep track of whether to overwrite the output buffers
    // for the voices.  this means we can skip clearing the buffers,
    // saving useless iterations.
    bool firstUnfilteredVoice = true;
    bool firstHarshFilteredVoice = true;
    bool firstSoftFilteredVoice = true;

    //- ojf: voice processing
    for (Voice& voice : context->voices)
    {
        switch (voice.filterType)
        {
            case FILT_NONE: //- ojf: unfiltered voices
                nextVoiceSamples (
                    &voice,
                    *buffer,
                    firstUnfilteredVoice);
                firstUnfilteredVoice = false;
                break;
            case FILT_HARSH: //- ojf: harshly filtered voices
                nextVoiceSamples (
                    &voice,
                    context->harshFilterInput,
                    firstHarshFilteredVoice);
                firstHarshFilteredVoice = false;
                break;
            case FILT_SOFT: //- ojf: soft filtered voices
                nextVoiceSamples (
                    &voice,
                    context->softFilterInput,
                    firstSoftFilteredVoice);
                firstSoftFilteredVoice = false;
                break;
        }
    }

    //- ojf: harsh filter
    processLadderFilterSamples (
        &context->harshFilter_l,
        context->harshFilterInput.leftBuffer,
        buffer->leftBuffer);
    processLadderFilterSamples (
        &context->harshFilter_r,
        context->harshFilterInput.rightBuffer,
        buffer->rightBuffer);

    //- ojf: soft filter
    processLadderFilterSamples (
        &context->softFilter_l,
        context->softFilterInput.leftBuffer,
        buffer->leftBuffer);
    processLadderFilterSamples (
        &context->softFilter_r,
        context->softFilterInput.rightBuffer,
        buffer->rightBuffer);

    //- ojf: fade in at beginning of drone
    if (context->rampSamples < rampTime * context->sampleRate)
    {
        for (int i = 0; i < buffer->leftBuffer.len; i++)
        {
            f32 rampAmount = (f32) context->rampSamples / (rampTime * context->sampleRate);

            if (rampAmount <= 1)
            {
                buffer->leftBuffer[i] = rampAmount * buffer->leftBuffer[i];
                buffer->rightBuffer[i] = rampAmount * buffer->rightBuffer[i];
            }

            context->rampSamples += 1;
        }
    }
}
