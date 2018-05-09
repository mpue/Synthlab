/*
  ==============================================================================

  This is an automatically generated GUI class created by the Projucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Projucer version: 5.2.1

  ------------------------------------------------------------------------------

  The Projucer is part of the JUCE library.
  Copyright (c) 2017 - ROLI Ltd.

  ==============================================================================
*/

#pragma once

//[Headers]     -- You can add your own extra header files here --
#include "../JuceLibraryCode/JuceHeader.h"
#include "MasterChannelPanel.h"
#include "Mixer.h"
#include "AuxOut.h"
#include "AudioIn.h"
#include "AudioOut.h"

//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Projucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class MixerPanel  : public juce::Component,
                    public juce::ChangeListener,
                    public juce::Timer
{
public:
    //==============================================================================
    MixerPanel ();
    ~MixerPanel();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
    void timerCallback() override;
    std::vector<MasterChannelPanel*> getChannels();

    void removeAllChannels();
    void changeListenerCallback (juce::ChangeBroadcaster* source)  override;
    int addChannel(juce::String name, Mixer::Channel::Type channeltype);
    void removeChannel(int index);
    void setMixer(Mixer* mixer);
    int getNextFreeChannelIndex();
    std::vector<AudioIn*>& getInputChannels();
    std::vector<AudioOut*>& getOutputChannels();
    std::vector<AuxOut*>& getAuxChannels();
    bool channelIsValid(int channel);
    bool auxChannelIsValid(int channel, int subchannel);
    void clearChannels();
    //[/UserMethods]
    
    void paint (juce::Graphics& g) override;
    void resized() override;


private:
    //[UserVariables]   -- You can add your own custom variables in this section.
    std::vector<MasterChannelPanel*> channels;
    Mixer* mixer;
    juce::Viewport* mixerView = nullptr;

    
    std::vector<AudioOut*> outputChannels;
    std::vector<AudioIn*> inputChannels;
    std::vector<AuxOut*> auxChannels;
    //[/UserVariables]

    //==============================================================================



    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MixerPanel)
};

//[EndFile] You can add extra defines here...
//[/EndFile]
