/*
  ==============================================================================

    MainTabbedComponent.h
    Created: 4 Apr 2018 12:50:15pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

//==============================================================================
/*
*/
class MainTabbedComponent    : public TabbedComponent
{
public:
    MainTabbedComponent();
    ~MainTabbedComponent();

    void popupMenuClickOnTab (int tabIndex, const String& tabName) override;
    
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainTabbedComponent)
};
