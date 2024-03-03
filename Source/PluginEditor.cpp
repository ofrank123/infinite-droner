/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
InfiniteDronerAudioProcessorEditor::InfiniteDronerAudioProcessorEditor (InfiniteDronerAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (400, 300);
    startTimerHz(15);
}

InfiniteDronerAudioProcessorEditor::~InfiniteDronerAudioProcessorEditor()
{
}

//==============================================================================
void InfiniteDronerAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    JuceContext j = {
        .graphics = &g,
    };

    drawGraphics(&audioProcessor.context, &j);
}

void InfiniteDronerAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
}

void InfiniteDronerAudioProcessorEditor::timerCallback()
{
    repaint();
}
