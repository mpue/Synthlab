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
#include "Pin.h"
#include "Terminal.h"

//==============================================================================
/*
 */
class TerminalModule    : public Module, public Terminal
{
public:
    
    TerminalModule(TerminalModule::Direction dir, TerminalModule::Type type);
    ~TerminalModule();
    
    void paint (juce::Graphics& g) override;

    virtual void configurePins() override;
    virtual juce::String getCategory() override {
        return "Input / Output";
    }
    
    virtual void setName(juce::String name) override;
    
    virtual void process() override;
    
    void setDirection(Direction dir);
    TerminalModule::Direction getDirection();
    
    virtual juce::Array<juce::PropertyComponent*>& getProperties() override;
    virtual void createProperties() override;

    virtual void eventReceived(Event *e) override;
    
    virtual void setType(Terminal::Type type) override;
    
private:
    
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
    
    struct TypeListener : juce::Value::Listener
    {
        TypeListener (juce::Value& v, TerminalModule* module) : module(module),  value (v) { value.addListener (this); }
        ~TypeListener()  {}  // no need to remove the listener
        
        void valueChanged (juce::Value& value) override {
            Type t;
            
            if (value.toString().getIntValue() == 0) {
                t = TerminalModule::Type::AUDIO;
            }
            else if (value.toString().getIntValue() == 1) {
                t = TerminalModule::Type::EVENT;
            }
            else
            {
                t = TerminalModule::Type::VALUE;
            }
            module->setType(t);
       
        }
        TerminalModule* module;
        juce::Value value;
    };
    
    NameListener* nameListener;
    juce::Value* nameValue;

    TypeListener* typeListener;
    juce::Value* typeValue;
    
    juce::PropertyComponent* typeProp;
    
    
    Direction direction;
    juce::String name;
    bool editing = false;
    Pin* pin = nullptr;
    
    std::vector<Pin*> proxies;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TerminalModule)
};
