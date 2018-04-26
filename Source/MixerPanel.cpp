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
    //[/UserPaint]
}

void MixerPanel::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    //[UserResized] Add your own custom resize handling here..
    setBounds(0, 0, 90 * channels.size(),250);
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

void MixerPanel::addChannel(String name, Mixer::Channel::Type channeltype) {

    int index = 0;
    
    switch (channeltype) {
        case Mixer::Channel::IN:
            index = mixer->getNumInputs();
            break;
        case Mixer::Channel::OUT:
            index = mixer->getNumOutputs();
            break;
        case Mixer::Channel::AUX:
            index = mixer->getNumAuxBusses();
            break;
        default:
            
            break;
    }
    
    MasterChannelPanel* panel = new MasterChannelPanel();
    panel->setIndex(index);
    panel->setName (name);
    panel->addChangeListener(this);
    panel->setBounds (90 * channels.size(),0, 90, 250);

    Mixer::Channel* channel = new Mixer::Channel();
    channel->index = panel->getIndex();
    mixer->addChannel(channel);
    channel->channelType = channeltype;
    panel->setChannel(channel);
    
    channels.push_back(panel);
    addAndMakeVisible(panel);
    setBounds(0, 0, 90 * channels.size(),250);
    repaint();
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
