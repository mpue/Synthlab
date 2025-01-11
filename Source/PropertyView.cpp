/*
  ==============================================================================

  This is an automatically generated GUI class created by the Projucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Projucer version: 5.2.1

  ------------------------------------------------------------------------------

  The Projucer is part of the JUCE library.
  Copyright (c) 2017 - ROLI Ltd.

  ==============================================================================
*/

//[Headers] You can add your own extra header files here...
#include "SynthEditor.h"
//[/Headers]

#include "PropertyView.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
using juce::TabbedButtonBar;
using juce::TabbedComponent;
using juce::TextEditor;
using juce::WildcardFileFilter;
using juce::DirectoryContentsList;
using juce::File;
using juce::PropertyPanel;
using juce::Graphics;
using juce::Colour;
//[/MiscUserDefs]

//==============================================================================
PropertyView::PropertyView () : TimeSliceThread("PropertyWatcher")
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (propertyTab = new MainTabbedComponent (TabbedButtonBar::TabsAtTop));
    propertyTab->setTabBarDepth (30);
    propertyTab->setCurrentTabIndex (-1);

    addAndMakeVisible (tabbedComponent = new MainTabbedComponent (TabbedButtonBar::TabsAtTop));
    tabbedComponent->setTabBarDepth (30);
    tabbedComponent->setCurrentTabIndex (-1);


    //[UserPreSize]
    //[/UserPreSize]

    setSize (600, 400);
    addTimeSliceClient(this);
    
    //[Constructor] You can add your own custom stuff here..
    setSize(getParentWidth(), getParentHeight());

    propertyTab->addTab("Properties",juce::Colours::grey,propertyPanel = new PropertyPanel(), true);

    propertyPanel->setMessageWhenEmpty("Nothing to display.");
    propertyPanel->setColour(juce::PropertyComponent::labelTextColourId, juce::Colours::white);
    
    descriptionEditor = new TextEditor();
    descriptionEditor->setMultiLine(true);
    descriptionEditor->setReadOnly(true);
    descriptionEditor->setJustification(juce::Justification::horizontallyJustified);

    filter = new WildcardFileFilter("*.wav;*.aif;*.aiff;*.mp3;*.slb","*","*");
    directoryContents = new DirectoryContentsList(filter,*this);
    directoryContents->setIgnoresHiddenFiles(false);
    File initialDir = File::getSpecialLocation(File::SpecialLocationType::userHomeDirectory).getFullPathName()+"/Documents/samples";
    FileBrowserModel* model = new FileBrowserModel(directoryContents, initialDir);
    browser  = new ExtendedFileBrowser(File::getSpecialLocation(File::userHomeDirectory),filter,model);
    directoryContents->addChangeListener(browser);
    tabbedComponent->addTab("Browser",juce::Colours::grey, browser, false);
    tabbedComponent->addTab("Description",juce::Colours::grey, descriptionEditor, false);

    startThread();

    //[/Constructor]
}

PropertyView::~PropertyView()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    propertyTab = nullptr;
    tabbedComponent = nullptr;
    // descriptionEditor = nullptr;

    //[Destructor]. You can add your own custom destruction code here..
    removeTimeSliceClient(this);
    delete browser;
    stopThread(100);
    delete directoryContents;
    delete filter;
    //[/Destructor]
}

//==============================================================================
void PropertyView::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.fillAll (Colour (0xff222222));

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void PropertyView::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    propertyTab->setBounds (0, 0, proportionOfWidth (1.0000f), proportionOfHeight (0.5000f));
    tabbedComponent->setBounds (0, proportionOfHeight (0.5000f), proportionOfWidth (1.0000f), proportionOfHeight (0.5000f));
    //[UserResized] Add your own custom resize handling here..

    //[/UserResized]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...

void PropertyView::selectionChanged(Component* c) {

    propertyPanel->clear();
    
    if (c != nullptr) {
        
        Module* m = dynamic_cast<Module*>(c);
    
        if (m != nullptr) {
            
            if (m->getSelectedPin() != nullptr){
                descriptionEditor->setText(m->getSelectedPin()->getDescription());
            }
            else {
                descriptionEditor->setText(m->getDescription());
            }
            
            propertyPanel->addSection("Module settings", m->getProperties());
            
            // propertyPanel->addProperties(m->getProperties());
        }

        AudioRegion* ar = dynamic_cast<AudioRegion*>(c);

        if (ar != nullptr) {
            // propertyPanel->addSection("Region settings", ar->getProperties());
        }
        else {
            Track* t = dynamic_cast<Track*>(c);

            if (t != nullptr) {
                propertyPanel->addSection("Track settings", t->getProperties());
            }
        }
    }

}

void PropertyView::fileChanged(juce::String name) {
    // propertyPanel->clear();
}

int PropertyView::useTimeSlice() {
    return 1000;
}

ExtendedFileBrowser* PropertyView::getBrowser() {
    return browser;
}

//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Projucer information section --

    This is where the Projucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="PropertyView" componentName=""
                 parentClasses="public Component, public ChangeListener" constructorParams=""
                 variableInitialisers="" snapPixels="8" snapActive="1" snapShown="1"
                 overlayOpacity="0.330" fixedSize="0" initialWidth="600" initialHeight="400">
  <BACKGROUND backgroundColour="ff323e44"/>
  <TABBEDCOMPONENT name="propertyTab" id="5a5a0866ea539d50" memberName="propertyTab"
                   virtualName="MainTabbedComponent" explicitFocusOrder="0" pos="0 0 100% 50%"
                   orientation="top" tabBarDepth="30" initialTab="-1"/>
  <TABBEDCOMPONENT name="MainTabbedComponent" id="c2d84f4a6b1e8bb1" memberName="tabbedComponent"
                   virtualName="" explicitFocusOrder="0" pos="0 50% 100% 50%" orientation="top"
                   tabBarDepth="30" initialTab="-1"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
//[/EndFile]
