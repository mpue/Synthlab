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
#include "Tracks/Track.h"
#include "Tracks/Region.h"
#include "Tracks/AudioRegion.h"
#include "Tracks/MidiRegion.h"
#include "PluginModule.h"

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

    // Palette wie in TrackControl — damit PropertyView visuell zum Rest der
    // dunklen DAW-UI passt, statt die JUCE-Defaults durchscheinen zu lassen.
    // panelBg/stripBg matchen MainComponent (0xff222222), damit PropertyView
    // und ihre Controls nahtlos im Parent-Hintergrund liegen.
    const juce::Colour panelBg   = juce::Colour(0xff222222);
    const juce::Colour stripBg   = juce::Colour(0xff222222);
    const juce::Colour btnOff    = juce::Colour(0xff1c1f25);
    const juce::Colour textHi    = juce::Colour(0xffe8e9ec);
    const juce::Colour textLo    = juce::Colour(0xffa8acb4);
    const juce::Colour accent    = juce::Colour(0xff5a8fff);
    const juce::Colour separator = juce::Colour(0xff2a2d33);

    auto styleTabs = [&](MainTabbedComponent& tc)
    {
        tc.setColour(juce::TabbedComponent::backgroundColourId,         panelBg);
        tc.setColour(juce::TabbedComponent::outlineColourId,            separator);
        tc.setColour(juce::TabbedButtonBar::tabOutlineColourId,         separator);
        tc.setColour(juce::TabbedButtonBar::tabTextColourId,            textLo);
        tc.setColour(juce::TabbedButtonBar::frontOutlineColourId,       accent.withAlpha(0.6f));
        tc.setColour(juce::TabbedButtonBar::frontTextColourId,          textHi);
    };

    propertyTab = std::make_unique<MainTabbedComponent>(TabbedButtonBar::TabsAtTop);

    addAndMakeVisible (propertyTab.get());
    propertyTab->setTabBarDepth (30);
    propertyTab->setCurrentTabIndex (-1);
    styleTabs(*propertyTab);

    tabbedComponent = std::make_unique<MainTabbedComponent>(TabbedButtonBar::TabsAtTop);

    addAndMakeVisible (tabbedComponent.get());
    tabbedComponent->setTabBarDepth (30);
    tabbedComponent->setCurrentTabIndex (-1);
    styleTabs(*tabbedComponent);

    //[UserPreSize]
    //[/UserPreSize]

    setSize (600, 400);
    addTimeSliceClient(this);
    
    //[Constructor] You can add your own custom stuff here..
    setSize(getParentWidth(), getParentHeight());

    propertyTab->addTab("Properties", stripBg, propertyPanel = new PropertyPanel(), true);

    // Custom L&F bypasses ElegantDarkLookAndFeel's chartreuse/red LinearBar
    // gradient (designed for synth-module knobs) and adds the right gutter
    // + narrower label allotment that keeps slider strips usable in slim
    // property panes. Paint overrides live in PropertyPanelLookAndFeel.h;
    // the colour table there mirrors the ones set right below — kept here
    // explicitly too so a future refactor that drops the L&F still renders
    // the strip backgrounds correctly.
    propertyPanelLnf = std::make_unique<PropertyPanelLookAndFeel>();
    propertyPanel->setLookAndFeel(propertyPanelLnf.get());

    propertyPanel->setMessageWhenEmpty("Nothing to display.");
    propertyPanel->setColour(juce::PropertyComponent::labelTextColourId,   textHi);
    propertyPanel->setColour(juce::PropertyComponent::backgroundColourId,  stripBg);

    // JUCE faerbt die Sub-Controls (BooleanPropertyComponent-Button,
    // TextPropertyComponent-Editor) NICHT ueber PropertyComponent::
    // backgroundColourId — die haben eigene Color-IDs mit default-tuerkis/
    // hellblauen JUCE-Werten. Auf parent-bg setzen, sonst bleibt das
    // "haessliche Dunkeltuerkis" stehen. PropertyPanel selbst hat keine
    // eigenen Color-IDs (Separatoren werden aus PropertyComponent::
    // backgroundColourId abgeleitet).
    propertyPanel->setColour(juce::BooleanPropertyComponent::backgroundColourId, stripBg);
    propertyPanel->setColour(juce::BooleanPropertyComponent::outlineColourId,    separator);

    propertyPanel->setColour(juce::TextPropertyComponent::backgroundColourId,    stripBg);
    propertyPanel->setColour(juce::TextPropertyComponent::textColourId,          textHi);
    propertyPanel->setColour(juce::TextPropertyComponent::outlineColourId,       separator);

    // SliderPropertyComponent: Text-Box + Track-Background auf parent-bg ziehen.
    propertyPanel->setColour(juce::Slider::backgroundColourId,         stripBg);
    propertyPanel->setColour(juce::Slider::textBoxBackgroundColourId,  stripBg);
    propertyPanel->setColour(juce::Slider::textBoxTextColourId,        textHi);
    propertyPanel->setColour(juce::Slider::textBoxOutlineColourId,     separator);

    // Eingebettete TextEditors (TextPropertyComponent nutzt intern einen) auf
    // parent-bg ziehen — sonst zieht die LookAndFeel ihren bgInput-Ton durch.
    propertyPanel->setColour(juce::TextEditor::backgroundColourId,     stripBg);
    propertyPanel->setColour(juce::TextEditor::textColourId,           textHi);
    propertyPanel->setColour(juce::TextEditor::outlineColourId,        separator);

    // ToggleButton-Tick (BooleanPropertyComponent) bleibt im Akzentton.
    propertyPanel->setColour(juce::ToggleButton::textColourId,         textHi);

    descriptionEditor = std::make_unique<TextEditor>("Description");
    descriptionEditor->setMultiLine(true);
    descriptionEditor->setReadOnly(true);
    descriptionEditor->setJustification(juce::Justification::horizontallyJustified);
    descriptionEditor->setColour(juce::TextEditor::backgroundColourId, stripBg);
    descriptionEditor->setColour(juce::TextEditor::textColourId,       textHi);
    descriptionEditor->setColour(juce::TextEditor::outlineColourId,    separator);
    descriptionEditor->setColour(juce::TextEditor::focusedOutlineColourId, accent.withAlpha(0.6f));
    descriptionEditor->applyFontToAllText(juce::Font(13.0f));

    filter = new WildcardFileFilter("*.wav;*.aif;*.aiff;*.mp3;*.slb","*","*");
    directoryContents = new DirectoryContentsList(filter,*this);
    directoryContents->setIgnoresHiddenFiles(false);
    File initialDir = File::getSpecialLocation(File::SpecialLocationType::userHomeDirectory).getFullPathName()+"/Documents/samples";
    model = new FileBrowserModel(directoryContents, initialDir);
    browser = std::make_unique<ExtendedFileBrowser>(File::getSpecialLocation(File::userHomeDirectory), filter, model);
    directoryContents->addChangeListener(browser.get());
    tabbedComponent->addTab("Browser", stripBg, browser.get(), false);

    // Instruments tab — lists every scanned virtual instrument and creates an
    // Instrument track on double-click. The editor pointer is wired later via
    // setEditor() (MainComponent does this once both objects exist), so the
    // callback checks for nullptr defensively.
    instrumentBrowser = std::make_unique<InstrumentBrowser>();
    instrumentBrowser->onInstrumentDoubleClicked = [this](const juce::String& name) {
        if (editor != nullptr)
            editor->addVirtualInstrumentTrack(name);
    };
    tabbedComponent->addTab("Instruments", stripBg, instrumentBrowser.get(), false);

    tabbedComponent->addTab("Description", stripBg, descriptionEditor.get(), false);

    showButton = std::make_unique<PulsingTextButton>("Show Plugin");
    addAndMakeVisible(*showButton);
    showButton->setColour(juce::TextButton::buttonColourId,   btnOff);
    showButton->setColour(juce::TextButton::buttonOnColourId, accent);
    showButton->setColour(juce::TextButton::textColourOffId,  textHi);
    showButton->setColour(juce::TextButton::textColourOnId,   juce::Colours::white);
    showButton->setVisible(false);
    showButton->onClick = [this]()
        {
            if (selectedTrack != nullptr)
            {
				selectedTrack->getPluginModule()->openPluginWindow();
            }

        };

    startThread();

    //[/Constructor]
}

PropertyView::~PropertyView()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    // Detach the custom L&F before propertyTab (which owns propertyPanel) is
    // reset. setLookAndFeel(nullptr) walks the children and clears their
    // pointers; doing this AFTER the panel is destroyed would dereference
    // a freed component. Order matters.
    if (propertyPanel != nullptr)
        propertyPanel->setLookAndFeel(nullptr);
    //[/Destructor_pre]

    propertyTab = nullptr;
    tabbedComponent = nullptr;
    descriptionEditor = nullptr;
    browser = nullptr;
    instrumentBrowser = nullptr;

    //[Destructor]. You can add your own custom destruction code here..
    removeTimeSliceClient(this);
    stopThread(100);
    delete directoryContents;
    delete filter;
    delete model;

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
    if (showButton)
        showButton->setBounds(getWidth() - 110, propertyTab->getBottom() - 35, 100, 25);

    //[/UserResized]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...

void PropertyView::selectionChanged(Component* c)
{
    propertyPanel->clear();
    selectedTrack = nullptr;
    showButton->setVisible(false);  // standardm��ig verstecken

    if (c != nullptr)
    {
        if (auto* m = dynamic_cast<Module*>(c))
        {
            descriptionEditor->setText(m->getSelectedPin() ? m->getSelectedPin()->getDescription() : m->getDescription());
            propertyPanel->addSection("Module settings", m->getProperties());
        }

        if (auto* t = dynamic_cast<Track*>(c))
        {
            selectedTrack = t;
            propertyPanel->addSection("Track settings", t->getProperties());

            if (selectedTrack->getType() == Track::Type::INSTRUMENT)
            {
                showButton->setVisible(true);
            }
        }

        // Region-Properties anzeigen, wenn TrackNavigator eine Region als
        // Selektion durchreicht. Vorher fiel das durch — AudioRegion zeigte
        // damit weder Name/Pitch noch die neuen Tempo-Follow Properties.
        if (auto* r = dynamic_cast<Region*>(c))
        {
            const char* sectionName = dynamic_cast<AudioRegion*>(r) != nullptr
                ? "Audio Region"
                : (dynamic_cast<MidiRegion*>(r) != nullptr ? "MIDI Region" : "Region");
            propertyPanel->addSection(sectionName, r->getProperties());
        }
    }

    resized(); // f�r korrekte Positionierung
}


void PropertyView::pulseShowPluginButton (Track* trackToSelect)
{
    if (showButton == nullptr)
        return;

    if (trackToSelect != nullptr)
        selectedTrack = trackToSelect;

    showButton->setVisible (true);
    resized();
    showButton->startPulse (3);
}

void PropertyView::fileChanged(juce::String name) {
    // propertyPanel->clear();
}

int PropertyView::useTimeSlice() {
    return 1000;
}

ExtendedFileBrowser* PropertyView::getBrowser() {
    return browser.get();
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
