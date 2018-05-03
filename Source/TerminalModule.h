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

//==============================================================================
/*
 */
class TerminalModule    : public Module
{
public:
    
    typedef enum TDirection {
        IN,
        OUT
    } Direction;
    
    TerminalModule(TerminalModule::Direction dir);
    
    void paint (Graphics& g) override;

    virtual void configurePins() override;
    virtual String getCategory() override {
        return "Input / Output";
    }
    
    void setDirection(Direction dir);
    TerminalModule::Direction getDirection();
    
private:
    Direction direction;
    String name;
    bool editing = false;
    Pin* pin = nullptr;
    
    std::vector<Pin*> proxies;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TerminalModule)
};
