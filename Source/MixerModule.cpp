/*
 ==============================================================================
 
 MixerMOdule
 Created: 4 Apr 2018 3:48:23pm
 Author:  Matthias Pueski
 
 ==============================================================================
 */

#include "../JuceLibraryCode/JuceHeader.h"
#include "MixerModule.h"
#include "Connection.h"


//==============================================================================
MixerModule::MixerModule(float sampleRate, int bufferSize)
{
    setSize(120,140);
    nameLabel->setJustificationType (Justification::left);
    nameLabel->setTopLeftPosition(18,2);
    setName("Mixer");
    editable = false;
    prefab = true;
    this->buffersize = bufferSize;
    this->sampleRate = sampleRate;
    createProperties();
}

MixerModule::~MixerModule()
{
    delete inputsValue;
    delete inputsListener;
}

void MixerModule::configurePins() {
    for (int i = 0;i < numInputs;i++) {
        Pin* p = new Pin(Pin::Type::AUDIO);
        p->direction = Pin::Direction::IN;
        p->setName("In"+String(i+1));
        
        Pin* pa = new Pin(Pin::Type::VALUE);
        pa->direction = Pin::Direction::IN;
        pa->setName("A"+(String(i+1)));
        
        addPin(Pin::Direction::IN,p);
        p->listeners.push_back(this);
        addPin(Pin::Direction::IN,pa);
        pa->listeners.push_back(this);
    }
    
    out = new Pin(Pin::Type::AUDIO);
    out->direction = Pin::Direction::OUT;
    out->setName("Out");
    out->listeners.push_back(this);
    
    addPin(Pin::Direction::OUT,out);

    out->getAudioBuffer()->clear();
    
    inputsValue->setValue(3);
}

void MixerModule::paint(juce::Graphics &g) {
    Module::paint(g);
    g.drawImageAt(ImageCache::getFromMemory(BinaryData::module_png, BinaryData::module_pngSize),25,40);
}

void MixerModule::process() {
    
    float* pOut = out->getAudioBuffer()->getWritePointer(0);

    for (int j = 0; j < buffersize;j++) {
        pOut[j] = 0;
        for (int i = 0; i < pins.size();i++) {
            if (pins.at(i)->getType() == Pin::Type::AUDIO && pins.at(i)->direction == Pin::Direction::IN && pins.at(i)->getConnections().size() == 1) {
                
                const float *in = pins.at(i)->getConnections().at(0)->getAudioBuffer()->getReadPointer(0);
                float value = 0;
                if  (pins.at(i+1)->getConnections().size() == 1) {
                    value = pins.at(i+1)->getConnections().at(0)->getValue();
                    pOut[j] += in[j] * value;
                }
                
                
            }
        }
    }
    
}

void MixerModule::setInputs(int newInputs) {
    if (newInputs > this->numInputs) {

        for (int i = numInputs; i < newInputs;i++) {
            Pin* p = new Pin(Pin::Type::AUDIO);
            p->direction = Pin::Direction::IN;
            p->setName("In"+String(i+1));
            
            Pin* pa = new Pin(Pin::Type::VALUE);
            pa->direction = Pin::Direction::IN;
            pa->setName("A"+(String(i+1)));
            
            addPin(Pin::Direction::IN,p);
            p->listeners.push_back(this);
            addPin(Pin::Direction::IN,pa);
            pa->listeners.push_back(this);
        }
        
        numInputs = newInputs;
        
        setSize(getWidth(), 40 * numInputs);
        repaint();
    }

}

void MixerModule::createProperties() {
    inputsValue = new Value();
    inputsListener = new InputsListener(*inputsValue, this);
}

juce::Array<PropertyComponent*>& MixerModule::getProperties() {
    
    properties = juce::Array<PropertyComponent*>();
    
    inputsProp = new SliderPropertyComponent(*inputsValue,"Inputs",1,16,1,1,true);
    properties.add(inputsProp);
    return properties;
}
