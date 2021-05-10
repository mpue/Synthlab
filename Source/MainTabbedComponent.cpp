/*
  ==============================================================================

    MainTabbedComponent.cpp
    Created: 4 Apr 2018 12:50:15pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "MainTabbedComponent.h"
#include "CloseableTabButton.h"

using juce::TabbedButtonBar;
using juce::Button;
using juce::TabbedButtonBar;
using juce::Colour;
using juce::PopupMenu;
using juce::TabBarButton;
using juce::String;
using juce::ImageButton;
using juce::ImageCache;

MainTabbedComponent::MainTabbedComponent() : juce::TabbedComponent(TabbedButtonBar::Orientation::TabsAtTop) , Button::Listener()
{
    
}

MainTabbedComponent::MainTabbedComponent(TabbedButtonBar::Orientation orientation) : juce::TabbedComponent(TabbedButtonBar::Orientation::TabsAtTop){

}


void MainTabbedComponent::paint(juce::Graphics &g) {
    // g.fillAll (Colour (0xff2e373c));
    g.setColour(Colours::grey);
    // g.drawRect(getBounds());
    g.drawLine(getWidth(), 0, getWidth(), getHeight());
    g.drawLine(0, getHeight(), getWidth(), getHeight());
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
        if (getNumTabs() > 0) {
            setCurrentTabIndex(getNumTabs() - 1);
        }
    }
    delete m;
 
}

void MainTabbedComponent::buttonClicked(juce::Button * button) {
    TabbedComponent::removeTab(button->getName().getIntValue());
    if (getNumTabs() > 0) {
        setCurrentTabIndex(getNumTabs() - 1);
    }
}

TabBarButton* MainTabbedComponent::createTabButton(const juce::String &tabName, int tabIndex)
{
    TabBarButton* tb = new TabBarButton(tabName,getTabbedButtonBar());
    tb->setName(String(tabIndex));
    tb->addMouseListener(this, true);
    
    ImageButton* ib = new ImageButton();
    ib->setName(String(tabIndex));
    ib->setSize(16,16);
    ib->setImages (false,
                   true,
                   true,
                   ImageCache::getFromMemory(BinaryData::close_png, BinaryData::close_pngSize),
                   1.0,
                   Colours::white,
                   ImageCache::getFromMemory(BinaryData::close_png, BinaryData::close_pngSize),
                   1.0,
                   Colours::orange,
                   ImageCache::getFromMemory(BinaryData::close_png, BinaryData::close_pngSize),
                   1.0,
                   Colours::orange);
    tb->setExtraComponent(ib, juce::TabBarButton::ExtraComponentPlacement::afterText);
    ib->addListener(this);
    return tb;
}


void MainTabbedComponent::addTab (const String& tabName,
             Colour tabBackgroundColour,
             Component* contentComponent,
             bool deleteComponentWhenNotNeeded) {
    
    // IEditor* editor = dynamic_cast<IEditor*>(contentComponent);
    
    editorMap.push_back(contentComponent);
    
    TabbedComponent::addTab(tabName, tabBackgroundColour, contentComponent, deleteComponentWhenNotNeeded);
}

bool MainTabbedComponent::isInterestedInDragSource(const juce::DragAndDropTarget::SourceDetails &dragSourceDetails) {
    /*
    TabbedButtonBar* tbb = dynamic_cast<TabbedButtonBar*>(dragSourceDetails.sourceComponent.get());
    
    if (tbb != nullptr) {

        MainTabbedComponent* tab = dynamic_cast<MainTabbedComponent*>(tbb->getParentComponent());
        
        if (tab != nullptr) {
             return true;
        }
        
       
    }
     */
    return false;
}

void MainTabbedComponent::itemDropped(const juce::DragAndDropTarget::SourceDetails &dragSourceDetails) {
    TabbedButtonBar* tbb = dynamic_cast<TabbedButtonBar*>(dragSourceDetails.sourceComponent.get());
    if (tbb != nullptr) {
        
        int index = tbb->getCurrentTabIndex();
        
        MainTabbedComponent* tab = dynamic_cast<MainTabbedComponent*>(tbb->getParentComponent());
        
        if (tab != nullptr) {
            Component* c = tab->getComponentAt(index);
            String name = tab->getTabNames().getReference(index);
            tab->removeTab(index);
            addTab(name, Colours::darkgrey, c, false);
        }
 
    }
}

Component* MainTabbedComponent::getComponentAt(int index) {
    return editorMap.at(index);
}


void MainTabbedComponent::mouseDoubleClick(const juce::MouseEvent &event) {
    fullSize = !fullSize;
    sendChangeMessage();
}
