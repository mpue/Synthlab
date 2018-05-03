//
//  TerminalModule.hpp
//  Synthlab - App
//
//  Created by Matthias Pueski on 03.05.18.
//  Copyright © 2018 Pueski. All rights reserved.
//

#pragma once
#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"

//==============================================================================
/*
 */
class TerminalModule    : public Module
{
public:
    TerminalModule();
    ~TerminalModule();
    
    void paint (Graphics& g) override;
    
    void setValue(float value);
    float getValue();
    virtual void configurePins() override;
    
    virtual void setName(String name)  {
        //Module::setName(name);
        Component::setName(name);
        this->name = name;
    }
    
    virtual String getName()  {
        return name;
    }
    
    virtual String getCategory() override {
        return "Input / Output";
    }
    
    struct ValueListener : Value::Listener
    {
        ValueListener (Value& v, TerminalModule* c) : c(c),  value (v) { value.addListener (this); }
        ~ValueListener()  {}  // no need to remove the listener
        
        void valueChanged (Value& value) override {
            c->setValue(value.toString().getFloatValue());
        }
        TerminalModule* c;
        Value value;
    };
    
    virtual juce::Array<PropertyComponent*>& getProperties() override;
    virtual void createProperties() override;
    
private:
    
    Value* valueValue;
    PropertyComponent* valueProp;
    ValueListener* valueListener;
    
    float value = 0;
    String name;
    bool editing = false;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TerminalModule)
};
