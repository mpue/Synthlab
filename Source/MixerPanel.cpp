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

//[Headers] You can add your own extra header files here...

//[/Headers]

#include "MixerPanel.h"

using juce::Graphics;
using juce::ChangeBroadcaster;
using juce::String;

//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
MixerPanel::MixerPanel ()
{
    //[Constructor_pre] You can add your own custom stuff here..


    //[/Constructor_pre]

 

    //[UserPreSize]
    //[/UserPreSize]

    setSize (600, 400);


    //[Constructor] You can add your own custom stuff here..

    //[/Constructor]
}

MixerPanel::~MixerPanel()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    
    for (std::vector<MasterChannelPanel*>::iterator it = channels.begin();it != channels.end();) {
        (*it)->setLookAndFeel(nullptr);
        (*it)->removeAllChangeListeners();
        delete (*it);
        it = channels.erase(it);

    }
    //[/Destructor_pre]

    //[/Destructor]
}

//==============================================================================
void MixerPanel::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    //[UserPaint] Add your own custom painting code here..
    g.fillAll (Colour (0xff222222));
    //[/UserPaint]
}

void MixerPanel::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    //[UserResized] Add your own custom resize handling here..
    setBounds(0, 0, 90 * static_cast<int>(channels.size()),250);
    //[/UserResized]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...

std::vector<MasterChannelPanel*> MixerPanel::getChannels() {
    return channels;
}


void MixerPanel::timerCallback() {
    /*
    for (int i = 0; i < mcc->getNavigator()->getTracks().size();i++) {
        Track* t = mcc->getNavigator()->getTracks().at(i);
        channels.at(i)->setMagnitude(t->magnitudeLeft);
    }
    masterChannel->setMagnitude(mcc->getMagnitude(0));
    */
}

void MixerPanel::changeListenerCallback (ChangeBroadcaster* source) {
    
    MasterChannelPanel* panel = nullptr;
    
    panel = dynamic_cast<MasterChannelPanel*>(source);
    
    if (panel != nullptr) {
        
    }
    
}

void MixerPanel::setMixer(Mixer *mixer) {
    this->mixer = mixer;
}

int MixerPanel::addChannel(String name, Mixer::Channel::Type channeltype) {

    int index = getNextFreeChannelIndex();

    MasterChannelPanel* panel = new MasterChannelPanel();
    panel->setIndex(index);
    panel->setName (name);
    panel->addChangeListener(this);
    panel->setBounds (90 * static_cast<int>(channels.size()),0, 90, 250);

    Mixer::Channel* channel = new Mixer::Channel();
    channel->index = panel->getIndex();
    mixer->addChannel(channel);
    channel->channelType = channeltype;
    panel->setChannel(channel);
    
    channels.push_back(panel);
    addAndMakeVisible(panel);
    setBounds(0, 0, 90 * static_cast<int>(channels.size()),250);
    repaint();
    
    return index;
}

int MixerPanel::getNextFreeChannelIndex() {
    int index = 0;
    
    for (int i = 0; i < channels.size();i++) {
        if (channels.at(i)->getIndex() > index) {
            index = channels.at(i)->getIndex();
        }
    }
    index++;
    return index;
}

void MixerPanel::removeChannel(int index) {
    for (std::vector<MasterChannelPanel*>::iterator it = channels.begin();it != channels.end();) {
        if((*it)->getIndex() == index) {
            (*it)->setLookAndFeel(nullptr);
            (*it)->removeAllChangeListeners();
            delete (*it);
            it = channels.erase(it);
            mixer->removeChannel(index);
        }
        else {
            ++it;
        }

        
    }
}

void MixerPanel::removeAllChannels() {
    mixer->removeAllChannels();
    for (std::vector<MasterChannelPanel*>::iterator it = channels.begin();it != channels.end();) {
        (*it)->setLookAndFeel(nullptr);
        (*it)->removeAllChangeListeners();
        delete (*it);
        it = channels.erase(it);
        
    }
}

bool MixerPanel::channelIsValid(int channel) {
    if (outputChannels.at(0)->getPins().at(channel)->getConnections().size() == 1 &&
        outputChannels.at(0)->getPins().at(channel)->getConnections().at(0) != nullptr &&
        outputChannels.at(0)->getPins().at(channel)->getConnections().at(0)->getAudioBuffer() != nullptr) {
        return true;
    }
    return false;
}

bool MixerPanel::auxChannelIsValid(int channel, int subchannel) {
    if (auxChannels.at(channel)->getPins().at(subchannel)->getConnections().size() == 1 &&
        auxChannels.at(channel)->getPins().at(subchannel)->getConnections().at(0) != nullptr &&
        auxChannels.at(channel)->getPins().at(subchannel)->getConnections().at(0)->getAudioBuffer() != nullptr) {
        return true;
    }
    return false;
}

std::vector<AudioIn*>& MixerPanel::getInputChannels() {
    return inputChannels;
}

std::vector<AudioOut*>& MixerPanel::getOutputChannels() {
    return outputChannels;
}

std::vector<AuxOut*>& MixerPanel::getAuxChannels() {
    return auxChannels;
}

void MixerPanel::clearChannels() {
    outputChannels.clear();
    inputChannels.clear();
    auxChannels.clear();
}
//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Projucer information section --

    This is where the Projucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="MixerPanel" componentName=""
                 parentClasses="public Component, public ChangeListener, public Timer"
                 constructorParams="" variableInitialisers="" snapPixels="8" snapActive="1"
                 snapShown="1" overlayOpacity="0.330" fixedSize="0" initialWidth="600"
                 initialHeight="400">
  <BACKGROUND backgroundColour="808080"/>
  <GENERICCOMPONENT name="Master" id="dac96aabc1eedcee" memberName="masterChannel"
                    virtualName="MasterChannelPanel" explicitFocusOrder="0" pos="90 0 90 250"
                    class="MasterChannelPanel" params=""/>
  <GENERICCOMPONENT name="Aux 1" id="fbda51a6d9971c30" memberName="aux1" virtualName="MasterChannelPanel"
                    explicitFocusOrder="0" pos="0 0 90 250" class="MasterChannelPanel"
                    params=""/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
//[/EndFile]
