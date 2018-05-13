 /*
 ==============================================================================
 
 SawtoothModule
 Created: 4 Apr 2018 3:48:23pm
 Author:  Matthias Pueski
 
 ==============================================================================
 */

#include "../JuceLibraryCode/JuceHeader.h"
#include "PluginModule.h"
#include "Connection.h"
#include "AudioManager.h"
#include "Plugins/PluginManager.h"

using juce::Justification;
using juce::AudioBuffer;
using juce::String;
using juce::ImageCache;
using juce::MidiMessage;
using juce::Time;
using juce::Value;
using juce::Array;
using juce::PropertyComponent;
using juce::var;
using juce::MemoryOutputStream;
using juce::Base64;
using juce::ChoicePropertyComponent;
using juce::MemoryBlock;

//==============================================================================
PluginModule::PluginModule(double sampleRate, int buffersize)
{
    this->sampleRate = sampleRate;
    this->buffersize = buffersize;
    
    setSize(120,140);
    nameLabel->setJustificationType (Justification::left);
    nameLabel->setTopLeftPosition(18,2);
    
    setName("Plugin");

    editable = false;
    prefab = true;
    
    bufferLeft = new float[buffersize];
    bufferRight = new float[buffersize];
    
    audioBuffer = new AudioBuffer<float>(2,buffersize);
    createProperties();
}

PluginModule::~PluginModule()
{
    delete bufferLeft;
    delete bufferRight;
    delete audioBuffer;
    delete pluginValue;
    delete pluginListener;
}

void PluginModule::openPluginWindow() {
    win = PluginManager::getInstance()->getPluginWindow(pluginName);
    if (win != nullptr) {
        win->setVisible(true);
    }
}

void PluginModule::selectPlugin(String name) {
    if (name == pluginName) {
        return;
    }
    // PluginManager::getInstance()->addPlugin(name, Project::getInstance()->getDeviceManager());
    // plugin = PluginManager::getInstance()->getPlugin(name);
    // PluginManager::getInstance()->openPluginWindow(name, Project::getInstance()->getDeviceManager());
    PluginManager::getInstance()->addPlugin(name, AudioManager::getInstance()->getDeviceManager());
    plugin = PluginManager::getInstance()->getPlugin(name);
    plugin->prepareToPlay(sampleRate, buffersize);
    pluginName = name;
    pluginValue->setValue(pluginName);
    //PluginManager::getInstance()->getPluginWindow(name)->setVisible(true);
}

String PluginModule::getPluginName() {
    return pluginName;
}

void PluginModule::configurePins() {
    
    Pin* p1 = new Pin(Pin::Type::AUDIO);
    p1->direction = Pin::Direction::IN;
    p1->listeners.push_back(this);
    p1->setName("L");
    
    Pin* p2 = new Pin(Pin::Type::AUDIO);
    p2->direction = Pin::Direction::IN;
    p2->listeners.push_back(this);
    p2->setName("R");
    
    Pin* p3 = new Pin(Pin::Type::AUDIO);
    p3->direction = Pin::Direction::OUT;
    p3->listeners.push_back(this);
    p3->setName("L");
    
    Pin* p4 = new Pin(Pin::Type::AUDIO);
    p4->direction = Pin::Direction::OUT;
    p4->listeners.push_back(this);
    p4->setName("R");
    
    Pin* p5 = new Pin(Pin::Type::EVENT);
    p5->direction = Pin::Direction::IN;
    p5->listeners.push_back(this);
    p5->setName("G");
    
    addPin(Pin::Direction::IN,p1);
    addPin(Pin::Direction::IN,p2);
    addPin(Pin::Direction::OUT,p3);
    addPin(Pin::Direction::OUT,p4);
    addPin(Pin::Direction::IN,p5);
}

void PluginModule::paint(juce::Graphics &g) {
    Module::paint(g);
    g.drawImageAt(ImageCache::getFromMemory(BinaryData::plugin_png, BinaryData::plugin_pngSize),25,40);
}


void PluginModule::process() {
    
    if (pins.at(0)->getConnections().size() == 1 && pins.at(1)->getConnections().size() == 1) {
        const float* inL = pins.at(0)->getConnections().at(0)->getAudioBuffer()->getReadPointer(0);
        const float* inR = pins.at(1)->getConnections().at(0)->getAudioBuffer()->getReadPointer(0);
        
        
        for (int i = 0; i < buffersize;i++){
            bufferLeft[i] = inL[i];
            bufferRight[i] = inR[i];
        }
        
        if (plugin != nullptr) {
            audioBuffer->copyFrom(0,0, *pins.at(0)->getConnections().at(0)->getAudioBuffer(), 0, 0, buffersize);
            audioBuffer->copyFrom(1,0, *pins.at(1)->getConnections().at(0)->getAudioBuffer(), 0, 0, buffersize);
            plugin->processBlock(*audioBuffer, midiBuffer);
            pins.at(2)->getAudioBuffer()->copyFrom(0,0, *audioBuffer, 0, 0, buffersize);
            pins.at(3)->getAudioBuffer()->copyFrom(0,0, *audioBuffer, 1, 0, buffersize);
        }
        
        /*
        for (int i = 0; i < buffersize;i++){
            pins.at(2)->getAudioBuffer()->setSample(0, i,bufferLeft[i]);
            pins.at(3)->getAudioBuffer()->setSample(0, i,bufferRight[i]);
        }
         */
        currentSample = (currentSample+1) % buffersize;
    }

    
}

void PluginModule::eventReceived(Event *e) {
    
    if (e->getType() == Event::Type::GATE) {
    
        float velocity = (1.0/127) * e->getValue();
        
        if (e->getValue() > 0) {
            MidiMessage start = MidiMessage::midiStart();
            start.setTimeStamp(Time::getMillisecondCounter());
            midiBuffer.addEvent(start,currentSample);
            MidiMessage* m = new MidiMessage(MidiMessage::noteOn((int)1, (int)e->getNote(), velocity));
            m->setTimeStamp(Time::getMillisecondCounter());
            midiBuffer.addEvent(*m, currentSample);
            
        }
        else {
            midiBuffer.addEvent(MidiMessage::midiStop(),currentSample);
            MidiMessage* m = new MidiMessage(MidiMessage::noteOff((int)1, (int)e->getNote(), velocity));
            m->setTimeStamp(Time::getMillisecondCounter());
            midiBuffer.addEvent(*m, currentSample);
        }
    }
    else if (e->getType() == Event::Type::CLOCK) {

        MidiMessage* m = new MidiMessage(MidiMessage::midiClock());
        m->setTimeStamp(Time::getMillisecondCounter());
        midiBuffer.addEvent(*m, currentSample);
    }
}

void PluginModule::createProperties() {

    pluginValue = new Value();
    pluginListener = new PluginValueListener(*pluginValue, this);

}

juce::Array<PropertyComponent*>& PluginModule::getProperties() {
    
    Array<var> values = Array<var>();
    
    for (int i = 0; i < PluginManager::getInstance()->getPlugins().size();i++) {
        values.add(PluginManager::getInstance()->getPlugins().getReference(i));
    }
    
    pluginProp = new ChoicePropertyComponent(*pluginValue,"Plugin",PluginManager::getInstance()->getPlugins(),values);
    properties = juce::Array<PropertyComponent*>();
    properties.add(pluginProp);
    return properties;
}

int PluginModule::getCurrentProgram() {
    if (plugin != nullptr) {
        currentProgram = plugin->getCurrentProgram();
    }
    return currentProgram;
}

void PluginModule::setCurrentProgram(int program) {
    currentProgram = program;
    
    if (plugin != nullptr) {
        plugin->setCurrentProgram(currentProgram);
    }
    
}

void PluginModule::setPluginState(juce::String state) {
    MemoryOutputStream* mos = new MemoryOutputStream();
    Base64::convertFromBase64(*mos, state);
    if (plugin != nullptr) {
        plugin->setStateInformation(mos->getData(), static_cast<int>(mos->getDataSize()));
    }
    delete mos;
}

String PluginModule::getPluginState() {
    if (plugin != nullptr) {
        MemoryBlock mb = MemoryBlock(65536,true);
        plugin->getStateInformation(mb);
        void* data = mb.getData();
        return Base64::toBase64(data , mb.getSize());
        
    }
    return "";
}
