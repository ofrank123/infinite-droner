// Copyright (c) 2024 Oliver Frank
// Licensed under the GNU Public License (https://www.gnu.org/licenses/)

#include "PluginEditor.h"
#include "PluginProcessor.h"

//==============================================================================
InfiniteDronerAudioProcessorEditor::InfiniteDronerAudioProcessorEditor (InfiniteDronerAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (400, 300);
}

InfiniteDronerAudioProcessorEditor::~InfiniteDronerAudioProcessorEditor()
{
}

//==============================================================================
void InfiniteDronerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void InfiniteDronerAudioProcessorEditor::resized()
{
}
