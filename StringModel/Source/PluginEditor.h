/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "PluginProcessor.h"
#include "Customize.h"
//#include "theString.h"
//==============================================================================
/**
*/
class StringModelAudioProcessorEditor  : public AudioProcessorEditor
{
public:
    StringModelAudioProcessorEditor (StringModelAudioProcessor&);
    ~StringModelAudioProcessorEditor();

    //==============================================================================
    void paint (Graphics&) override;
    void buttonClicked(int seq);
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    StringModelAudioProcessor& processor;

    //Customize cusGUI;
   // theString myString;
    Label title;
    Customize stringCUS;
    Customize drumCUS;
    Customize boxCUS;
    //Customize* cusGUI;
    DrawableButton stringButton;
    DrawableButton drumButton;
    DrawableButton boxButton;
    Image background;
    TabbedComponent mode;
    ScopedPointer<AudioProcessorValueTreeState::ButtonAttachment> dimSelection;
    ScopedPointer<AudioProcessorValueTreeState::ButtonAttachment> stringTree;
    ScopedPointer<AudioProcessorValueTreeState::ButtonAttachment> drumTree;
    ScopedPointer<AudioProcessorValueTreeState::ButtonAttachment> boxTree;
    Button* but;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StringModelAudioProcessorEditor)
};
