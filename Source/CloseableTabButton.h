/*
  ==============================================================================

    CloseableTabButton.h
    Created: 25 Apr 2018 5:51:34pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

class CloseableTabButton : public juce::TabBarButton {
public:
    CloseableTabButton(const juce::String& name, juce::TabbedButtonBar& ownerBar);
    ~CloseableTabButton();
    
};
