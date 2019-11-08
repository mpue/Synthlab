/*
  ==============================================================================

    MainTabbedComponent.h
    Created: 4 Apr 2018 12:50:15pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "IEditor.h"
#include <map>

//==============================================================================
/*
*/
class MainTabbedComponent    : public juce::TabbedComponent,
                               public juce::Button::Listener,
                               public DragAndDropTarget,
                               public ChangeBroadcaster
{
public:
    MainTabbedComponent();
    MainTabbedComponent(juce::TabbedButtonBar::Orientation orientation);
    void paint (juce::Graphics& g) override;
    void popupMenuClickOnTab (int tabIndex, const juce::String& tabName) override;
    juce::TabBarButton* createTabButton (const juce::String& tabName, int tabIndex) override;
    virtual void buttonClicked (juce::Button*) override;
    void addTab (const juce::String& tabName,
                 juce::Colour tabBackgroundColour,
                 juce::Component* contentComponent,
                 bool deleteComponentWhenNotNeeded);
    virtual void mouseDoubleClick (const MouseEvent& event) override;
    void itemDropped (const SourceDetails& dragSourceDetails) override;
    bool isInterestedInDragSource (const SourceDetails& dragSourceDetails) override;
    Component* getComponentAt(int index);
    bool isFullSize() {
        return fullSize;
    }
private:
    
    std::vector<Component*> editorMap;
    bool fullSize = false;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainTabbedComponent)
};
