/*
  ==============================================================================

    FileButtonPropertyComponent.h
    Created: 15 May 2018 11:03:07am
    Author:  Matthias Pueski

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

class FileButtonPropertyComponent : public ButtonPropertyComponent {
    
public:
    
    FileButtonPropertyComponent(String text, Value& valueToControl);
    ~FileButtonPropertyComponent();
    
    virtual void buttonClicked() override;
    virtual String getButtonText() const override;
    
    String buttonText;
    Value& value;
    
};
