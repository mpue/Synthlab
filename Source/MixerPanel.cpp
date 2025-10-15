#include "MixerPanel.h"

using juce::Graphics;
using juce::ChangeBroadcaster;
using juce::String;

MixerPanel::MixerPanel ()
{
	outputChannels.reserve(2);
	inputChannels.reserve(2);
	auxChannels.reserve(2);
    setSize(600, 400);
}

MixerPanel::~MixerPanel()
{
    for (std::vector<MasterChannelPanel*>::iterator it = channels.begin();it != channels.end();) {
        (*it)->setLookAndFeel(nullptr);
        (*it)->removeAllChangeListeners();
        delete (*it);
        it = channels.erase(it);
    }
}

void MixerPanel::paint (Graphics& g)
{
    g.fillAll (Colour (0xff1a1a1a));
}

void MixerPanel::resized()
{
    setBounds(0, 0, 90 * static_cast<int>(channels.size()),250);
 }

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

int MixerPanel::addChannel(String name, Mixer::Channel::Type channeltype) {

    int index = getNextFreeChannelIndex();

    MasterChannelPanel* panel = new MasterChannelPanel();
    panel->setIndex(index);
    panel->setName (name);
    panel->addChangeListener(this);
    panel->setBounds (90 * static_cast<int>(channels.size()),0, 90, 250);

    Mixer::Channel* channel = new Mixer::Channel();
    channel->index = panel->getIndex();
    Mixer::getInstance()->addChannel(channel);
    channel->channelType = channeltype;
    panel->setChannel(channel);
    
    channels.push_back(panel);
    addAndMakeVisible(panel);
    setBounds(0, 0, 90 * static_cast<int>(channels.size()),250);
    repaint();
    
    return index;
}

void MixerPanel::setVolume(int channelIndex, float volume) {
    for (int i = 0; i < channels.size();i++) {
        if (channels.at(i)->getIndex() == channelIndex){
            channels.at(i)->setVolume(volume);
            break;
        }
    }
}

void MixerPanel::setModule(int channelIndex, VolumeAdjustable* v) {
    for (int i = 0; i < channels.size();i++) {
        if (channels.at(i)->getIndex() == channelIndex){
            channels.at(i)->setModule(v);
            v->addChangeListener(channels.at(i));
            break;
        }
    }
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
            Mixer::getInstance()->removeChannel(index);
        }
        else {
            ++it;
        }        
    }
}

void MixerPanel::removeAllChannels() {
    Mixer::getInstance()->removeAllChannels();
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
	
	if (outputChannels.size() > 0)
		outputChannels.clear();
    
	if (inputChannels.size() > 0) 
		inputChannels.clear();

	if (auxChannels.size() > 0)
		auxChannels.clear();

}
