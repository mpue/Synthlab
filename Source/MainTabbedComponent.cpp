/*
  ==============================================================================

    MainTabbedComponent.cpp
    Created: 4 Apr 2018 12:50:15pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "MainTabbedComponent.h"

//==============================================================================
MainTabbedComponent::MainTabbedComponent() : juce::TabbedComponent(TabbedButtonBar::Orientation::TabsAtTop)
{
    // In your constructor, you should add any child components, and
    // initialise any special settings that your component needs.

}

MainTabbedComponent::MainTabbedComponent(TabbedButtonBar::Orientation orientation) : juce::TabbedComponent(TabbedButtonBar::Orientation::TabsAtTop){
    
}

MainTabbedComponent::~MainTabbedComponent()
{
}

void MainTabbedComponent::popupMenuClickOnTab(int tabIndex, const juce::String &tabName) {
    
    if (tabIndex == 0){
        return;
    }
    
    PopupMenu* m = new PopupMenu();
    
    m->addItem(1, "Close tab");

    const int result = m->show();
    
    if (result == 0)
    {
        // user dismissed the menu without picking anything
    }
    else if (result == 1) {
        TabbedComponent::removeTab(tabIndex);
    }
    
    delete m;
 
}
