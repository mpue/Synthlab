/*
  ==============================================================================

    CloseableTabButton.h
    Created: 25 Apr 2018 5:51:34pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

class CloseableTabButton : public TabBarButton {
public:
    CloseableTabButton(const String& name, TabbedButtonBar& ownerBar);
    ~CloseableTabButton();
    
};
