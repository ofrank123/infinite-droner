// Copyright (c) 2024 Oliver Frank
// Licensed under the GNU Public License (https://www.gnu.org/licenses/)

#pragma once

#include "PluginProcessor.h"
#include <JuceHeader.h>

class InfiniteDronerAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    InfiniteDronerAudioProcessorEditor (InfiniteDronerAudioProcessor&);
    ~InfiniteDronerAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;

    void resized() override;

private:
    InfiniteDronerAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InfiniteDronerAudioProcessorEditor)
};
