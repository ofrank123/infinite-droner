#include "Oscillator.h"

//- ojf: here lie the wavetables, generated in WaveTables.m matlab script
#include "./tables_N2048_f40_o9.h"
//- ojf: constants associated with the wavetable
const usize wavetable_samples = 2048;
const f32 wavetable_f0 = 40;
const f32 wavetable_octaves = 9;

internal inline void updatePhase(Oscillator *osc, f32 frequencyMod) {
    f32 phaseDelta = (osc->frequency + frequencyMod) / osc->sampleRate;
    osc->phase += phaseDelta;

    while(osc->phase > 1) {
        osc->phase -= 1;
    }
}

internal inline f32 nextSineSample(
    Oscillator *osc,
    f32 frequencyMod,
    f32 amplitudeMod
) {
    updatePhase(osc, frequencyMod); 
    return amplitudeMod * sin(TWO_PI * osc->phase);
}

internal inline void nextSineSamples(
    Oscillator *osc,
    StereoBuffer output,
    bool useFreqMod,
    Slice<f32> frequencyModulation,
    bool useAmpMod,
    Slice<f32> amplitudeModulation,
    bool overwrite,
    bool mono,
    f32 amplitude
) {
    for (int i=0; i<output.leftBuffer.len; i++) {
        f32 sample = nextSineSample(
            osc, 
            useFreqMod ? frequencyModulation[i] : 0, 
            amplitude + (useAmpMod ? amplitudeModulation[i] : 0)
        );

        if (overwrite) {
            output.leftBuffer[i] = sample;
            if (!mono) {
                output.rightBuffer[i] = sample;
            }
        } else {
            output.leftBuffer[i] += sample;
            if (!mono) {
                output.rightBuffer[i] += sample;
            }
        }
    }
}

internal inline void nextNoiseSamples(
    Oscillator *osc,
    StereoBuffer output,
    bool useAmpMod,
    Slice<f32> amplitudeModulation,
    bool overwrite,
    bool mono,
    f32 amplitude
) {
    for (int i=0; i<output.leftBuffer.len; i++) {
        f32 sample = ((f32) rand() / (f32) RAND_MAX) * 2.0 - 1.0;
        sample *= amplitude + (useAmpMod ? amplitudeModulation[i] : 0);

        if (overwrite) {
            output.leftBuffer[i] = sample;
            if (!mono) {
                output.rightBuffer[i] = sample;
            }
        } else {
            output.leftBuffer[i] += sample;
            if (!mono) {
                output.rightBuffer[i] += sample;
            }
        }
    }
}

internal inline f32 nextTableSample(
    Oscillator *osc,
    f32 frequencyMod,
    f32 amplitudeMod,
    const float *table
) {
    updatePhase(osc, frequencyMod);

    f32 table_offset = osc->octave * wavetable_samples;
    f32 table_idx = table_offset + osc->phase * wavetable_samples;

    f32 table_sample_l = table[(usize) floor(table_idx)];
    f32 table_sample_r = table[(usize) ceil(table_idx)];

    return amplitudeMod * 0.5 * (table_sample_l + table_sample_r);
}

internal inline void _sampleTable(
    Oscillator *osc, 
    StereoBuffer output,
    bool useFreqMod,
    Slice<f32> frequencyModulation,
    bool useAmpMod,
    Slice<f32> amplitudeModulation,
    bool overwrite,
    bool mono,
    f32 amplitude,
    const float *table
) {
    //- ojf: this loop (and the sine wave loop) appears to have
    // a lot of branches that could be precalculated.  to solve
    // this, i created a more optimized (or so i thought)
    // version, that precomputed the branches, and had
    // made the permutations of the loops with a macro.  obviously
    // this was.
    for (int i=0; i<output.leftBuffer.len; i++) {
        f32 sample = nextTableSample(
            osc, 
            useFreqMod ? frequencyModulation[i] : 0, 
            amplitude + (useAmpMod ? amplitudeModulation[i] : 0),
            table
        );
        
        if (overwrite) {
            output.leftBuffer[i] = sample;
            if (!mono) {
                output.rightBuffer[i] = sample;
            }
        } else {
            output.leftBuffer[i] += sample;
            if (!mono) {
                output.rightBuffer[i] += sample;
            }
        }
    }
}

internal void _nextOscillatorSamples(
    Oscillator *osc,
    StereoBuffer output,
    bool useFreqMod,
    Slice<f32> frequencyMod,
    bool useAmpMod,
    Slice<f32> amplitudeMod,
    bool overwrite,
    bool mono,
    f32 amplitude
) {
    switch (osc->type) {
        case OSC_SINE: {
            nextSineSamples(
                osc, 
                output, 
                useFreqMod,
                frequencyMod,
                useAmpMod,
                amplitudeMod,
                overwrite,
                mono,
                amplitude
            );
            break;
        }
        case OSC_NOISE: {
            nextNoiseSamples(
                osc, 
                output, 
                useAmpMod,
                amplitudeMod,
                overwrite,
                mono,
                amplitude
            );
            break;
        }
        case OSC_SQUARE: {
            _sampleTable(
                osc, 
                output, 
                useFreqMod,
                frequencyMod,
                useAmpMod,
                amplitudeMod,
                overwrite,
                mono,
                amplitude, 
                square_N2048_f40_o9
            );
            break;
        } case OSC_SAW: {
            _sampleTable(
                osc, 
                output, 
                useFreqMod,
                frequencyMod,
                useAmpMod,
                amplitudeMod,
                overwrite,
                mono,
                amplitude, 
                saw_N2048_f40_o9
            );
            break;
        }
        case OSC_TRIANGLE: {
            _sampleTable(
                osc, 
                output, 
                useFreqMod,
                frequencyMod,
                useAmpMod,
                amplitudeMod,
                overwrite,
                mono,
                amplitude, 
                triangle_N2048_f40_o9
            );
            break;
        }
    }
}

void nextOscillatorSamplesMono(
    Oscillator *osc,
    Slice<f32> output,
    bool useFreqMod,
    Slice<f32> frequencyMod,
    bool useAmpMod,
    Slice<f32> amplitudeMod,
    bool overwrite,
    f32 amplitude
) {
    _nextOscillatorSamples(
        osc, 
        { .leftBuffer = output }, 
        useFreqMod,
        frequencyMod,
        useAmpMod,
        amplitudeMod,
        overwrite,
        true, 
        amplitude
    );
}

void nextOscillatorSamples(
    Oscillator *osc,
    StereoBuffer output,
    bool useFreqMod,
    Slice<f32> frequencyMod,
    bool useAmpMod,
    Slice<f32> amplitudeMod,
    bool overwrite,
    f32 amplitude
) {
    _nextOscillatorSamples(
        osc, 
        output, 
        useFreqMod,
        frequencyMod,
        useAmpMod,
        amplitudeMod,
        overwrite,
        false, 
        amplitude
    );
}

void setOscillatorFrequency(Oscillator *osc, f32 frequency) {
    osc->frequency = frequency;
    osc->phaseDelta = osc->frequency / osc->sampleRate;

    usize octave = 0; 
    if (frequency > wavetable_f0) {
        f32 f0 = wavetable_f0;

        for (int n=0; n <= wavetable_octaves; n++) {
            octave = n;
            if (frequency < f0*2) {
                break;
            }
            f0 *= 2;
        }
    }

    osc->octave = octave;
}

Oscillator createOscillator(OscillatorType type, f32 sampleRate, f32 frequency) {
    Oscillator osc = {
        .type = type,
        .sampleRate = sampleRate
    };

    setOscillatorFrequency(&osc, frequency);
    return osc;
}

Lfo createLfo(OscillatorType type, f32 sampleRate, usize blockSize, f32 frequency, f32 depth) {
    return {
        .mod = createSlice(blockSize),
        .osc = createOscillator(type, sampleRate, frequency),
        .depth = depth,
    };
}

//------------------------------
//~ ojf: voice processing

void nextVoiceSamples(Voice *voice, StereoBuffer output, bool overwrite) {
    if (voice->enableMetaFrequencyLfo) {
        nextOscillatorSamplesMono(
            &voice->metaFrequencyLfo.osc,
            voice->metaFrequencyLfo.mod,
            false, {},
            false, {},
            true,
            voice->metaFrequencyLfo.depth
        );
    }

    if (voice->enableFrequencyLfo) {
        nextOscillatorSamplesMono(
            &voice->frequencyLfo.osc,
            voice->frequencyLfo.mod,
            voice->enableMetaFrequencyLfo, voice->metaFrequencyLfo.mod,
            false, {},
            true,
            voice->frequencyLfo.depth
        );
    }

    if (voice->enableMetaAmplitudeLfo) {
        nextOscillatorSamplesMono(
            &voice->metaAmplitudeLfo.osc,
            voice->metaAmplitudeLfo.mod,
            false, {},
            false, {},
            true,
            voice->metaAmplitudeLfo.depth
        );
    }
    if (voice->enableAmplitudeLfo) {
        nextOscillatorSamplesMono(
            &voice->amplitudeLfo.osc,
            voice->amplitudeLfo.mod,
            voice->enableMetaAmplitudeLfo, voice->metaAmplitudeLfo.mod,
            false, {},
            true,
            voice->amplitudeLfo.depth
        );
    }

    nextOscillatorSamples(
        &voice->oscillator, 
        output, 
        voice->enableFrequencyLfo, voice->frequencyLfo.mod,
        voice->enableAmplitudeLfo, voice->amplitudeLfo.mod,
        overwrite,
        voice->volume);
}

