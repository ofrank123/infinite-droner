#include "Plugin.h"
#include "Oscillator.h"

#include <cassert>
#include <cmath>
#include <cstring>

//------------------------------
//~ ojf: memory

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

void clearSlice(Slice<f32> slice) {
    memset(slice.ptr, 0, sizeof(f32) * slice.len);
}

void clearStereoBuffer(StereoBuffer buffer) {
    clearSlice(buffer.leftBuffer);
    clearSlice(buffer.rightBuffer);
}

Slice<f32> createSlice(usize len) {
    return {
        .ptr = (f32 *) calloc(len, sizeof(f32)),
        .len = len,
    };
}

StereoBuffer createStereoBuffer(usize len) {
    return {
        .leftBuffer = createSlice(len),
        .rightBuffer = createSlice(len),
    };
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
//~ ojf: initialization + cleanup

void init(PluginContext *context, f32 sampleRate, usize samplesPerBlock) {
    context->sampleRate = sampleRate;
    context->timeStep = sampleRate;
    context->samplesPerBlock = samplesPerBlock;
    context->blockSamples = {
        .ptr = (f32 *) calloc(samplesPerBlock, sizeof(f32)),
        .len = samplesPerBlock,
    };

    context->harshFilterInput = createStereoBuffer(samplesPerBlock);
    context->softFilterInput = createStereoBuffer(samplesPerBlock);

    const f32 master_level = 0.8;
    { //- ojf: sub
        context->voices.push_back({
            .volume = 0.2f * master_level,
            .filterType = FILT_NONE,
            .oscillator = createOscillator(
                OSC_SINE, sampleRate, 50
            ),
        });
        context->voices.push_back({
            .volume = 0.1f * master_level,
            .filterType = FILT_NONE,
            .oscillator = createOscillator(
                OSC_SINE, sampleRate, 40
            ),
        });
    }

    { //- ojf: noise
        context->voices.push_back({
            .volume = 0.1f * master_level,
            .filterType = FILT_HARSH,
            .oscillator = createOscillator(
                OSC_NOISE, sampleRate, 40
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
                    OSC_SAW, sampleRate, 50 + i * 50.2
                ),
                .enableAmplitudeLfo = true,
                .amplitudeLfo = createLfo(
                    OSC_SAW, 
                    sampleRate, 
                    samplesPerBlock, 
                    0.001 + i * 0.001, 
                    0.02
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
            .enableFrequencyLfo = true,
            .frequencyLfo = createLfo(OSC_SINE, sampleRate, samplesPerBlock, 2, 1),
        });
        context->voices.push_back({
            .volume = 0.2f * master_level,
            .filterType = FILT_SOFT,
            .oscillator = createOscillator(OSC_TRIANGLE, sampleRate, 587.33),
            .enableMetaFrequencyLfo = true,
            .metaFrequencyLfo = createLfo(OSC_SINE, sampleRate, samplesPerBlock, 0.01, 2),
            .enableFrequencyLfo = true,
            .frequencyLfo = createLfo(OSC_SINE, sampleRate, samplesPerBlock, 0.5, 5),
            .enableAmplitudeLfo = true,
            .amplitudeLfo = createLfo(OSC_SAW, sampleRate, samplesPerBlock, 0.001, 0.4),
        });
        context->voices.push_back({
            .volume = 0.2f * master_level,
            .filterType = FILT_SOFT,
            .oscillator = createOscillator(OSC_TRIANGLE, sampleRate, 659.26),
            .enableMetaFrequencyLfo = true,
            .metaFrequencyLfo = createLfo(OSC_SINE, sampleRate, samplesPerBlock, 0.02, 1.5),
            .enableFrequencyLfo = true,
            .frequencyLfo = createLfo(OSC_SINE, sampleRate, samplesPerBlock, 0.6, 7),
            .enableAmplitudeLfo = true,
            .amplitudeLfo = createLfo(OSC_SAW, sampleRate, samplesPerBlock, 0.003, 0.4),
        });
    }

    { //- ojf chirps
        context->voices.push_back({
            .volume = 0.03f * master_level,
            .filterType = FILT_NONE,
            .oscillator = createOscillator(OSC_SINE, sampleRate, 700),
            .enableMetaFrequencyLfo = true,
            .metaFrequencyLfo = createLfo(OSC_SINE, sampleRate, samplesPerBlock, 0.001, 2),
            .enableFrequencyLfo = true,
            .frequencyLfo = createLfo(OSC_SINE, sampleRate, samplesPerBlock, 2000, 100),
            .enableMetaAmplitudeLfo = true,
            .metaAmplitudeLfo = createLfo(OSC_SQUARE, sampleRate, samplesPerBlock, 0.002, 0.2),
            .enableAmplitudeLfo = true,
            .amplitudeLfo = createLfo(OSC_SINE, sampleRate, samplesPerBlock, 0.02, 0.03),
        });
        context->voices.push_back({
            .volume = 0.05f * master_level,
            .filterType = FILT_NONE,
            .oscillator = createOscillator(OSC_SINE, sampleRate, 666),
            .enableMetaFrequencyLfo = true,
            .metaFrequencyLfo = createLfo(OSC_SINE, sampleRate, samplesPerBlock, 0.001, 2),
            .enableFrequencyLfo = true,
            .frequencyLfo = createLfo(OSC_SINE, sampleRate, samplesPerBlock, 1200, 100),
            .enableMetaAmplitudeLfo = true,
            .metaAmplitudeLfo = createLfo(OSC_SQUARE, sampleRate, samplesPerBlock, 0.0001, 0.2),
            .enableAmplitudeLfo = true,
            .amplitudeLfo = createLfo(OSC_SINE, sampleRate, samplesPerBlock, 0.001, 0.05),
        });
        context->voices.push_back({
            .volume = 0.05f * master_level,
            .filterType = FILT_SOFT,
            .oscillator = createOscillator(OSC_SAW, sampleRate, 1500),
            .enableFrequencyLfo = true,
            .frequencyLfo = createLfo(OSC_SINE, sampleRate, samplesPerBlock, 2, 0.02),
            .enableMetaAmplitudeLfo = true,
            .metaAmplitudeLfo = createLfo(OSC_SQUARE, sampleRate, samplesPerBlock, 0.0001, 0.2),
            .enableAmplitudeLfo = true,
            .amplitudeLfo = createLfo(OSC_SINE, sampleRate, samplesPerBlock, 0.001, 0.05),
        });
    }

    // gainey resonant filter
    context->harshFilter_l = {
        .res = 1.0f,
        .cutoff = 600.0f,
        .gain = 10.0f,
        .output_gain = 1.0f,
        .timestep = 1 / sampleRate,
        ._cutoffLfo = createLfo(OSC_SINE, sampleRate, samplesPerBlock, 0.004, 400),
        ._metaCutoffLfo = createLfo(OSC_SINE, sampleRate, samplesPerBlock, 0.005, 0.09f),
    };
    context->harshFilter_r = {
        .res = 1.0f,
        .cutoff = 600.0f,
        .gain = 10.0f,
        .output_gain = 1.0f,
        .timestep = 1 / sampleRate,
        ._cutoffLfo = createLfo(OSC_SINE, sampleRate, samplesPerBlock, 0.005, 405),
        ._metaCutoffLfo = createLfo(OSC_SINE, sampleRate, samplesPerBlock, 0.006, 0.087f),
    };

    context->softFilter_l = {
        .res = 0.2f,
        .cutoff = 2000.0f,
        .gain = 1.0f,
        .output_gain = 1.0f,
        .timestep = 1 / sampleRate,
        ._cutoffLfo = createLfo(OSC_SINE, sampleRate, samplesPerBlock, 0.03, 1000),
        ._metaCutoffLfo = createLfo(OSC_SINE, sampleRate, samplesPerBlock, 0.001, 0.02f),
    };
    context->softFilter_r = {
        .res = 0.3f,
        .cutoff = 2000.0f,
        .gain = 1.0f,
        .output_gain = 1.0f,
        .timestep = 1 / sampleRate,
        ._cutoffLfo = createLfo(OSC_SINE, sampleRate, samplesPerBlock, 0.025, 1000),
        ._metaCutoffLfo = createLfo(OSC_SINE, sampleRate, samplesPerBlock, 0.0015, 0.02f),
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

    //- ojf: keep track of whether to overwrite the output buffer
    bool firstUnfilteredVoice = true;
    bool firstHarshFilteredVoice = true;
    bool firstSoftFilteredVoice = true;

    for (Voice& voice : context->voices) {
        switch (voice.filterType) {
            case FILT_NONE:
                nextVoiceSamples(
                    &voice, 
                    *buffer, 
                    firstUnfilteredVoice
                );
                firstUnfilteredVoice = false;
                break;
            case FILT_HARSH:
                nextVoiceSamples(
                    &voice, 
                    context->harshFilterInput, 
                    firstHarshFilteredVoice
                );
                firstHarshFilteredVoice = false;
                break;
            case FILT_SOFT:
                nextVoiceSamples(
                    &voice, 
                    context->softFilterInput, 
                    firstSoftFilteredVoice
                );
                firstSoftFilteredVoice = false;
                break;
        }
    }

    //- ojf: harsh filter
    processLadderFilterSamples(
        &context->harshFilter_l,
        context->harshFilterInput.leftBuffer,
        buffer->leftBuffer
    );
    processLadderFilterSamples(
        &context->harshFilter_r,
        context->harshFilterInput.rightBuffer,
        buffer->rightBuffer
    );

    //- ojf: soft filter
    processLadderFilterSamples(
        &context->softFilter_l,
        context->softFilterInput.leftBuffer,
        buffer->leftBuffer
    );
    processLadderFilterSamples(
        &context->softFilter_r,
        context->softFilterInput.rightBuffer,
        buffer->rightBuffer
    );
}

//------------------------------
//~ ojf: main graphics call

void drawGraphics(PluginContext *context, JuceContext *j) {
    f32 width = 400;
    f32 height = 300;
    usize N = context->blockSamples.len;

    // context->samplesLock.lock();
    // {
    //     usize zeroPoint = 0;
    //     for (; zeroPoint < N; zeroPoint++) {
    //         if (fabs(context->blockSamples[zeroPoint]) <= 0.01) {
    //             break;
    //         }
    //     }

    //     usize end = umin(zeroPoint + N/2, N);
    //     usize range = end - zeroPoint;
    //     for (int i = zeroPoint; i < end; i++) {
    //         f32 startX = width * ((f32) (i - 1 - zeroPoint) / ((f32)range));
    //         f32 startY = 50 + 50 * context->blockSamples[i - 1];
    //         f32 endX = width * ((f32) (i - zeroPoint) / ((f32)range));
    //         f32 endY = 50 + 50 * context->blockSamples[i];
    //         drawLine(j, { startX, startY }, { endX, endY }, 0x0EB53B);
    //     }
    // }

    // {
    //     f32 specHeight = 150;
    //     Slice<f32> blockSamplesSpec =  {
    //         .ptr = (f32*) calloc(sizeof(f32), N),
    //         .len = N,
    //     };
    //     fft_db(context->blockSamples, blockSamplesSpec);
    //     f32 dbRange = 60;
    //     f32 maxSpecVal = sliceMax(blockSamplesSpec);
    //     f32 minSpecVal = maxSpecVal - dbRange;
    //     for (usize i = 1; i < N/2; i++) {
    //         f32 startX = width * ((f32) (i - 1) / ((f32)N/2));
    //         f32 startY = height - specHeight * 
    //             ((blockSamplesSpec[i - 1] - minSpecVal) / (dbRange));
    //         f32 endX = width * ((f32) i / ((f32)N/2));
    //         f32 endY = height - specHeight *
    //             ((blockSamplesSpec[i] - minSpecVal) / (dbRange));
    //         drawLine(j, { startX, startY }, { endX, endY }, 0x0EB53B);
    //     }
    //     free(blockSamplesSpec.ptr);
    // }

    // context->samplesLock.unlock();
}
