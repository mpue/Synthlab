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
    
    void paint (Graphics& g) override;

    virtual void configurePins() override;
    virtual String getCategory() override {
        return "Input / Output";
    }
    
    virtual void setName(String name) override;
    
    virtual void process() override;
    
    void setDirection(Direction dir);
    TerminalModule::Direction getDirection();
    
    virtual juce::Array<PropertyComponent*>& getProperties() override;
    virtual void createProperties() override;

    virtual void setType(Terminal::Type type) override;
    
private:
    
    struct NameListener : Value::Listener
    {
        NameListener (Value& v, Module* module) : module(module),  value (v) { value.addListener (this); }
        ~NameListener()  {}  // no need to remove the listener
        
        void valueChanged (Value& value) override {
            module->setName(value.toString());
        }
        Module* module;
        Value value;
    };
    
    struct TypeListener : Value::Listener
    {
        TypeListener (Value& v, TerminalModule* module) : module(module),  value (v) { value.addListener (this); }
        ~TypeListener()  {}  // no need to remove the listener
        
        void valueChanged (Value& value) override {
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
        Value value;
    };
    
    NameListener* nameListener;
    Value* nameValue;

    TypeListener* typeListener;
    Value* typeValue;
    
    PropertyComponent* typeProp;
    
    
    Direction direction;
    String name;
    bool editing = false;
    Pin* pin = nullptr;
    
    std::vector<Pin*> proxies;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TerminalModule)
};
