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

//==============================================================================
/*
*/
class Knob    : public Module, juce::Slider::Listener
{
public:
    Knob();
    ~Knob();

    void paint (Graphics& g) override;

    void setValue(float value);
    float getValue();
    virtual void configurePins() override;
    void sliderValueChanged (juce::Slider* slider) override;
    virtual juce::String getCategory() override {
        return "Controls";
    }
    
    void setStepSize(float stepsize);
    void setMinimum(float min);
    void setMaximum(float max);
    
    float getStepsize();
    float getMinimum();
    float getMaximum();
    
    bool isMidiController();
    void setIsMidicontroller(bool isController);
    
    virtual void setCurrentLayer(Layer layer) override;
    
    bool isLearning();
    void setLearning(bool learning);
    
    int getController();
    void setController(int num);
    void eventReceived(Event *e) override;
    
    virtual void setSelected(bool selected) override;
    
    virtual juce::Array<juce::PropertyComponent*>& getProperties() override;
    virtual void createProperties() override;
    
    float map(float x, float in_min, float in_max, float out_min, float out_max)
    {
        return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
    }
    
    
    virtual Layer getLayer() override {
        return Layer::GUI;
    };
    
private:
    
    struct ValueListener : juce::Value::Listener
    {
        ValueListener (juce::Value& v, Knob* k) : k(k),  value (v) { value.addListener (this); }
        ~ValueListener()  {}  // no need to remove the listener
        
        void valueChanged (juce::Value& value) override {
            k->setValue(value.toString().getFloatValue());
        }
        Knob* k;
        juce::Value value;
    };
    
    struct MaxValueListener : juce::Value::Listener
    {
        MaxValueListener (juce::Value& v, Knob* k) : k(k), value (v) { value.addListener (this); }
        ~MaxValueListener()  {}  // no need to remove the listener
        
        void valueChanged (juce::Value& value) override {
            k->setMaximum(value.toString().getFloatValue());
        }
        Knob* k;
        juce::Value value;
    };
    
    struct MinValueListener : juce::Value::Listener
    {
        MinValueListener (juce::Value& v, Knob* k) : k(k), value (v){ value.addListener (this); }
        ~MinValueListener()  {}  // no need to remove the listener
        
        void valueChanged (juce::Value& value) override {
            k->setMinimum(value.toString().getFloatValue());
        }
        Knob* k;
        juce::Value value;
    };
    
    struct StepsizeValueListener : juce::Value::Listener
    {
        StepsizeValueListener (juce::Value& v, Knob* k) : k(k), value (v){ value.addListener (this); }
        ~StepsizeValueListener()  {}  // no need to remove the listener
        
        void valueChanged (juce::Value& value) override {
            k->setStepSize(value.toString().getFloatValue());
        }
        Knob* k;
        juce::Value value;
    };
    
    struct IsControllerValueListener : juce::Value::Listener
    {
        IsControllerValueListener (juce::Value& v, Knob* k) : k(k), value (v){ value.addListener (this); }
        ~IsControllerValueListener()  {}  // no need to remove the listener
        
        void valueChanged (juce::Value& value) override {
            k->setIsMidicontroller(value.toString().getIntValue() > 0);
        }
        Knob* k;
        juce::Value value;
    };
    
    struct ControllerValueListener : juce::Value::Listener
    {
        ControllerValueListener (juce::Value& v, Knob* k) : k(k), value (v){ value.addListener (this); }
        ~ControllerValueListener()  {}  // no need to remove the listener
        
        void valueChanged (juce::Value& value) override {
            k->setController(value.toString().getIntValue());
        }
        Knob* k;
        juce::Value value;
    };
    
    struct NameListener : juce::Value::Listener
    {
        NameListener (juce::Value& v, Module* module) : module(module),  value (v) { value.addListener (this); }
        ~NameListener()  {}  // no need to remove the listener
        
        void valueChanged (juce::Value& value) override {
            module->setName(value.toString());
        }
        Module* module;
        juce::Value value;
    };
    
    float value;
    juce::Slider* slider;
    bool isController = false;
    int controllerNum = 1;
    bool learning = false;
    
    float minimum = 0 ;
    float maximum = 127;
    float stepsize = 1;

    juce::Value* valueValue;
    juce::Value* minValue;
    juce::Value* maxValue;
    juce::Value* stepsizeValue;
    juce::Value* isControllerValue;
    juce::Value* controllerValue;
    juce::Value* nameValue;
    
    juce::PropertyComponent* valueProp;
    juce::PropertyComponent* minValueProp;
    juce::PropertyComponent* maxValueProp;
    juce::PropertyComponent* stepsizeValueProp;
    juce::PropertyComponent* isControllerValueProp;
    juce::PropertyComponent* controllerValueProp;
    juce::PropertyComponent* nameValueProp;
    
    ValueListener* valueListener;
    MinValueListener* minValueListener;
    MaxValueListener* maxValueListener;
    StepsizeValueListener* stepsizeValueListener;
    IsControllerValueListener* isControllerValueListener;
    ControllerValueListener* controllerValueListener;
    NameListener* nameListener;

    
    Label* valueLabel = nullptr;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Knob)
};
