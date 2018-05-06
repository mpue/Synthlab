/*
 ==============================================================================
 
 MidiGate.h
 Created: 4 Apr 2018 3:48:23pm
 Author:  Matthias Pueski
 
 ==============================================================================
 */

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"
#include "AudioEngine/Oszillator.h"
#include "Monophonic.h"
//==============================================================================
/*
 */

template <typename T>
class OscillatorModule : public Module, public Monophonic
{
public:
    OscillatorModule(double sampleRate, int buffersize, juce::Image* image);
    ~OscillatorModule();
    
    void paint (juce::Graphics& g) override;
    
    void setPitch(int pitch);
    void setAmplitude(float amplitude);
    void setFine(float fine);
    void process() override;
    virtual void configurePins() override;
    
    virtual juce::String getCategory() override {
        return "Sound sources";
    }
    virtual juce::String getDescription() override {
        return BinaryData::oscillator_txt;
    }
    
    bool isMono() override;
    void setMono(bool value) override;
    
    void eventReceived(Event *e) override;
    
    virtual juce::Array<juce::PropertyComponent*>& getProperties() override;
    virtual void createProperties() override;
    
    
private:
    
    struct IsMonoListener : juce::Value::Listener
    {
        IsMonoListener (juce::Value& v, OscillatorModule* mod) : mod(mod),  value (v) { value.addListener (this); }
        ~IsMonoListener()  {}  // no need to remove the listener
        
        void valueChanged (juce::Value& value) override {
            mod->setMono(value.toString().getIntValue() > 0);
        }
        OscillatorModule* mod;
        juce::Value value;
    };
    
    juce::Value* isMonoValue;
    juce::PropertyComponent* isMonoProp;
    IsMonoListener* isMonoListener;
    
    juce::AudioSampleBuffer* out = nullptr;
    float frequencies[128] = {0};
    int pitch = 0;
    float amplitude;
    float fine = 0;
    juce::Image* image = nullptr;
    T* monoOscillator;
    T* oscillator[128];
    int currentSample = 0;
    bool mono;
    float pitchBend = 1;
    Pin* gatePin = nullptr;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OscillatorModule)
};


/*
 ==============================================================================
 
 OscillatorModule
 Created: 4 Apr 2018 3:48:23pm
 Author:  Matthias Pueski
 
 ==============================================================================
 */

#include "../JuceLibraryCode/JuceHeader.h"
#include "OscillatorModule.h"
#include "Connection.h"

//==============================================================================
template<typename T> OscillatorModule<T>::OscillatorModule(double sampleRate, int buffersize, juce::Image* image) : image(image)
{
    this->sampleRate = sampleRate;
    this->buffersize = buffersize;
    
    for (int i = 0; i < 128;i++){
        oscillator[i] = nullptr;
    }
    
    monoOscillator = new T(sampleRate, buffersize);
    
    setSize(120,140);
    nameLabel->setJustificationType (juce::Justification::left);
    nameLabel->setTopLeftPosition(18,2);

    editable = false;
    prefab = true;
    
}

template<typename T> OscillatorModule<T>::~OscillatorModule()
{
    
    for (int i = 0; i < 128;i++){
        if (oscillator[i] != nullptr)
            delete oscillator[i];
    }
    
    delete monoOscillator;
    if (image != nullptr) {
        delete image;
    }
    delete isMonoListener;
    delete isMonoValue;
    
}

template<typename T> void OscillatorModule<T>::configurePins() {
    Pin* p1 = new Pin(Pin::Type::VALUE);
    p1->direction = Pin::Direction::IN;
    p1->listeners.push_back(this);
    p1->setName("P");
    
    Pin* p2 = new Pin(Pin::Type::VALUE);
    p2->direction = Pin::Direction::IN;
    p2->listeners.push_back(this);
    p2->setName("F");
    
    Pin* p3 = new Pin(Pin::Type::VALUE);
    p3->direction = Pin::Direction::IN;
    p3->listeners.push_back(this);
    p3->setName("A");
    
    Pin* p4 = new Pin(Pin::Type::AUDIO);
    p4->direction = Pin::Direction::OUT;
    p4->listeners.push_back(this);
    p4->setName("Out");
    
    Pin* p5 = new Pin(Pin::Type::VALUE);
    p5->direction = Pin::Direction::OUT;
    p5->listeners.push_back(this);
    p5->setName("V");
    
    Pin* p6 = new Pin(Pin::Type::VALUE);
    p6->direction = Pin::Direction::IN;
    p6->listeners.push_back(this);
    p6->setName("Pb");
    
    addPin(Pin::Direction::IN,p1);
    addPin(Pin::Direction::IN,p2);
    addPin(Pin::Direction::IN,p3);
    addPin(Pin::Direction::OUT,p4);
    addPin(Pin::Direction::OUT,p5);
    addPin(Pin::Direction::IN,p6);
    
    createProperties();
    out = pins.at(3)->getAudioBuffer();
    
}

template<typename T> void OscillatorModule<T>::paint(juce::Graphics &g) {
    Module::paint(g);
    g.drawImageAt(*image,25,40);
}


template<typename T> void OscillatorModule<T>::setPitch(int pitch) {
    this->pitch = pitch;
}

template<typename T> void OscillatorModule<T>::setFine(float fine) {
    if (this->fine != fine) {
        this->fine = fine;
        for (int i = 0; i < 128;i++){
            if (oscillator[i] != nullptr)
                this->oscillator[i]->setFine(fine);
        }
    }
}

template<typename T> void OscillatorModule<T>::setAmplitude(float amplitude) {
    this->amplitude = amplitude;
}

template<typename T> void OscillatorModule<T>::setMono(bool value) {
    this->mono = value;
    this->isMonoValue->setValue(value);
}

template<typename T> bool OscillatorModule<T>::isMono() {
    return mono;
}

template<typename T> juce::Array<juce::PropertyComponent*>& OscillatorModule<T>::getProperties() {
    
    properties = juce::Array<juce::PropertyComponent*>();
    isMonoProp = new juce::BooleanPropertyComponent(*isMonoValue,"isMono","Mono");
    
    
    properties.add(isMonoProp);
    
    return properties;
}


template<typename T> void OscillatorModule<T>::createProperties() {
    isMonoValue = new juce::Value();
    isMonoListener = new IsMonoListener(*isMonoValue, this);
    isMonoValue->setValue(mono);
}

template<typename T> void OscillatorModule<T>::process() {
    bool volumegate = false;
    
    
    
    if (pins.at(0)->getConnections().size() ==  1) {
        this->setPitch(pins.at(0)->getConnections().at(0)->getValue());
    }
    if (pins.at(1)->getConnections().size() ==  1) {
        this->setFine(pins.at(1)->getConnections().at(0)->getValue());
    }
    if (pins.at(5)->getConnections().size() ==  1) {
        this->pitchBend = pins.at(5)->getConnections().at(0)->getValue();
        for (int j = 0; j < 128;j++){
            
            if(gatePin->dataEnabled[j]) {
                if (oscillator[j] != nullptr) {
                    oscillator[j]->setFrequency((440 * pow(2.0,((j+1+pitch)-69.0)/12.0)) * pitchBend);
                }
            }
        }
    }
    if (pins.at(2)->getConnections().size() ==  1) {
        volumegate = true;
        gatePin = pins.at(2)->getConnections().at(0);
    }
    if (pins.at(2)->getConnections().size() == 1) {
        for (int i = 0; i < buffersize; i++) {
            float value = 0;
            
            if (mono) {
                this->monoOscillator->setFrequency((440 * pow(2.0,((pitch)-69.0)/12.0)) * pitchBend);
                this->monoOscillator->setVolume(pins.at(2)->getConnections().at(0)->getValue());
                value = monoOscillator->process();
                pins.at(4)->setValue(abs(value + 1));
            }
            else {
                for (int j = 0; j < 128;j++){
                    
                    if(gatePin->dataEnabled[j]) {
                        if (oscillator[j] ==  nullptr) {
                            oscillator[j] = new T(sampleRate, buffersize);
                            
                            oscillator[j]->setFrequency((440 * pow(2.0,((j+1+pitch)-69.0)/12.0)) * pitchBend);
                        }
                        float volume = gatePin->data[j];
                        
                        this->oscillator[j]->setVolume(volume);
                    }
                    else {
                        if (oscillator[j] != nullptr) {
                            
                            delete this->oscillator[j];
                            this->oscillator[j] = nullptr;
                        }
                    }
                    if (oscillator[j] != nullptr)
                        value += oscillator[j]->process();
                }
            }
            if (out != nullptr && out->getNumChannels() > 0)
                out->setSample(0,i ,value);
            
            
            
        }
    }
    else {
        if (out != nullptr && out->getNumChannels() > 0) {
            out->clear();
        }
    }
    
}

template<typename T> void OscillatorModule<T>::eventReceived(Event *e) {
    if (e->getType() == Event::Type::PITCH) {
        pitchBend = e->getValue();
    }
}


