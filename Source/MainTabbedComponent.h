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
class MainTabbedComponent    : public TabbedComponent, public Button::Listener
{
public:
    MainTabbedComponent();
    MainTabbedComponent(TabbedButtonBar::Orientation orientation);
    void paint (Graphics& g) override;
    void popupMenuClickOnTab (int tabIndex, const String& tabName) override;
    TabBarButton* createTabButton (const String& tabName, int tabIndex) override;
    void buttonClicked (Button*);
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainTabbedComponent)
};
