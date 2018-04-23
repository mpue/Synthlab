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
#include "Project.h"
#include "Plugins/PluginManager.h"

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
    
    audioBuffer = new AudioBuffer<float>(4,buffersize);
    
}

PluginModule::~PluginModule()
{
    delete bufferLeft;
    delete bufferRight;
    delete plugin;
}

void PluginModule::openPluginWindow() {
    PluginManager::getInstance()->getPluginWindow(pluginName)->setVisible(true);
}

void PluginModule::selectPlugin(String name) {
    // PluginManager::getInstance()->addPlugin(name, Project::getInstance()->getDeviceManager());
    // plugin = PluginManager::getInstance()->getPlugin(name);
    // PluginManager::getInstance()->openPluginWindow(name, Project::getInstance()->getDeviceManager());
    PluginManager::getInstance()->addPlugin(name, Project::getInstance()->getDeviceManager());
    plugin = PluginManager::getInstance()->getPlugin(name);
    plugin->prepareToPlay(sampleRate, buffersize);
    //PluginManager::getInstance()->getPluginWindow(name)->setVisible(true);
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
    
    addPin(Pin::Direction::IN,p1);
    addPin(Pin::Direction::IN,p2);
    addPin(Pin::Direction::OUT,p3);
    addPin(Pin::Direction::OUT,p4);

}

void PluginModule::paint(juce::Graphics &g) {
    Module::paint(g);
    // g.drawImageAt(ImageCache::getFromMemory(BinaryData::lp_png, BinaryData::lp_pngSize),25,40);
}


void PluginModule::process() {
    
    if (pins.at(0)->connections.size() == 1 && pins.at(1)->connections.size() == 1) {
        const float* inL = pins.at(0)->connections.at(0)->getAudioBuffer()->getReadPointer(0);
        const float* inR = pins.at(1)->connections.at(0)->getAudioBuffer()->getReadPointer(0);
        
        for (int i = 0; i < buffersize;i++){
            bufferLeft[i] = inL[i];
            bufferRight[i] = inR[i];
        }
        
        if (plugin != nullptr) {
            audioBuffer->copyFrom(0,0, *pins.at(0)->connections.at(0)->getAudioBuffer(), 0, 0, buffersize);
            audioBuffer->copyFrom(1,0, *pins.at(1)->connections.at(0)->getAudioBuffer(), 0, 0, buffersize);
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
        
    }

    
}

