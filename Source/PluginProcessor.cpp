// Copyright (c) 2024 Oliver Frank
// Licensed under the GNU Public License (https://www.gnu.org/licenses/)

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cassert>

//==============================================================================
InfiniteDronerAudioProcessor::InfiniteDronerAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor (BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
#endif
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
#endif
    )
#endif
{
}

InfiniteDronerAudioProcessor::~InfiniteDronerAudioProcessor()
{
}

//==============================================================================
const juce::String InfiniteDronerAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool InfiniteDronerAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool InfiniteDronerAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool InfiniteDronerAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double InfiniteDronerAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int InfiniteDronerAudioProcessor::getNumPrograms()
{
    return 1; // NB: some hosts don't cope very well if you tell them there are 0 programs,
        // so this should be at least 1, even if you're not really implementing programs.
}

int InfiniteDronerAudioProcessor::getCurrentProgram()
{
    return 0;
}

void InfiniteDronerAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String InfiniteDronerAudioProcessor::getProgramName (int index)
{
    return {};
}

void InfiniteDronerAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void InfiniteDronerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    //- ojf: big reverb, which pairs nicely with the synths
    juce::Reverb::Parameters reverbParams = {
        .roomSize = 1.0f,
        .damping = 0.2f,
        .wetLevel = .60,
        .dryLevel = .40,
        .width = 1.0f,
    };

    //- ojf: initialize the plugin context
    init (&context, sampleRate, samplesPerBlock);
}

void InfiniteDronerAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
    cleanup (&context);
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool InfiniteDronerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
#else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

        // This checks if the input layout matches the output layout
#if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
#endif
}
#endif

void InfiniteDronerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    //- ojf: this is a synth, and we only handle 2 channels
    assert (totalNumInputChannels == 0);
    assert (totalNumOutputChannels == 2);

    const i32 numSamples = buffer.getNumSamples();
    assert (numSamples > 0);

    //- ojf: create main buffer
    StereoBuffer stereoBuffer = {
        .leftBuffer = {
            .ptr = buffer.getWritePointer (0),
            .len = (usize) numSamples,
        },
        .rightBuffer = {
            .ptr = buffer.getWritePointer (1),
            .len = (usize) numSamples,
        },
    };

    //- ojf: main processing function
    processSamples (&context, &stereoBuffer);

    //- ojf: quick and dirty reverb processing
    reverb.processStereo (
        stereoBuffer.leftBuffer.ptr,
        stereoBuffer.rightBuffer.ptr,
        numSamples);
}

//==============================================================================
bool InfiniteDronerAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* InfiniteDronerAudioProcessor::createEditor()
{
    return new InfiniteDronerAudioProcessorEditor (*this);
}

//==============================================================================
void InfiniteDronerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void InfiniteDronerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new InfiniteDronerAudioProcessor();
}
