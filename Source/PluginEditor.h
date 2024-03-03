#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class InfiniteDronerAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                       public juce::Timer
{
public:
    InfiniteDronerAudioProcessorEditor (InfiniteDronerAudioProcessor&);
    ~InfiniteDronerAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;

    void resized() override;

    void timerCallback() override;

private:
    InfiniteDronerAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InfiniteDronerAudioProcessorEditor)
};
