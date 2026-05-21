/*
  ==============================================================================

    TriggerScriptModule.cpp
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "TriggerScriptModule.h"
#include "JavaScriptEditorComponent.h"
#include "Project/Project.h"
#include "AudioEngine/Event.h"
#include "Pin.h"
#include "Connection.h"

using juce::Value;
using juce::PropertyComponent;
using juce::ChoicePropertyComponent;
using juce::StringArray;
using juce::Array;
using juce::var;
using juce::File;
using juce::Justification;

TriggerScriptModule::TriggerScriptModule()
{
    setSize (120, 80);
    setName ("Trigger Script");
    editable = false;
    prefab   = true;

    createProperties();
}

TriggerScriptModule::~TriggerScriptModule()
{
    delete pathListener;
    delete pathValue;
    // PropertyComponents are owned by the PropertyPanel after addSection - we
    // don't free them here. JUCE handles their lifetime.
}

void TriggerScriptModule::configurePins()
{
    gatePin = new Pin (Pin::Type::EVENT);
    gatePin->direction = Pin::Direction::IN;
    gatePin->listeners.push_back (this);
    gatePin->setName ("G");
    gatePin->setDescription ("Gate event input - gate-on fires the selected macro.");
    addPin (Pin::Direction::IN, gatePin);
}

void TriggerScriptModule::paint (juce::Graphics& g)
{
    Module::paint (g);

    // Show the bound macro's filename inside the module so a glance at the
    // patch tells you which script will fire. Empty path = italic "(none)".
    g.setColour (juce::Colours::white.withAlpha (0.85f));
    g.setFont (juce::Font (12.0f, macroPath.isEmpty() ? juce::Font::italic : juce::Font::plain));
    const auto label = macroPath.isEmpty()
                        ? juce::String ("(no macro)")
                        : File (macroPath).getFileNameWithoutExtension();
    g.drawFittedText (label, 6, getHeight() - 22, getWidth() - 12, 18,
                      Justification::centred, 1);
}

void TriggerScriptModule::setMacroPath (const juce::String& p)
{
    if (macroPath == p) return;
    macroPath = p;

    // Sync the Value so the property editor's ChoicePropertyComponent
    // reflects external changes (load-from-XML, JS API in the future).
    if (pathValue != nullptr && pathValue->toString() != p)
        pathValue->setValue (p);

    repaint();
}

void TriggerScriptModule::eventReceived (Event* eventIn)
{
    if (eventIn == nullptr) return;
    if (eventIn->getType() != Event::Type::GATE) return;

    // Rising edge only - fire on "gate on", ignore "gate off". Convention
    // established by TriggerTimerModule / MidiGate (Event::name set to
    // "gate on" / "gate off" by upstream modules).
    if (eventIn->getName() != "gate on") return;

    if (macroPath.isEmpty()) return;

    // We're called from the audio thread via Pin::sendEvent. Running a JS
    // script here would: (a) allocate (Duktape heap), (b) potentially open
    // modal dialogs (alert/prompt), (c) block the audio block. All forbidden.
    // Marshal to the message thread via callAsync.
    juce::String path = macroPath;  // copy for the lambda
    juce::MessageManager::callAsync ([path]()
    {
        if (auto* ed = Project::getInstance()->getJavaScriptEditor())
            ed->runFile (File (path));
    });
}

void TriggerScriptModule::createProperties()
{
    pathValue    = new Value();
    pathValue->setValue (macroPath);
    pathListener = new PathListener (*pathValue, this);
}

juce::Array<PropertyComponent*>& TriggerScriptModule::getProperties()
{
    properties = juce::Array<PropertyComponent*>();

    // Scan ~/.Synthlab/macros/ recursively for .js files. Rebuilt every time
    // the user selects the module so freshly-dropped macros appear without
    // restart. Empty dir is fine - ChoicePropertyComponent shows an empty
    // dropdown.
    StringArray choices;
    Array<var>  values;

    // First entry is always "(none)" with empty value so the user can unbind.
    choices.add ("(none)");
    values.add (var (juce::String()));

    const File macroDir = File::getSpecialLocation (File::userHomeDirectory)
                              .getChildFile (".Synthlab/macros");
    if (macroDir.isDirectory())
    {
        juce::Array<File> jsFiles;
        macroDir.findChildFiles (jsFiles, File::findFiles, /*recursive*/ true, "*.js");
        // Sort by name for a stable, predictable dropdown.
        struct ByName { int compareElements (const File& a, const File& b) const
                        { return a.getFileName().compareNatural (b.getFileName()); } };
        ByName cmp; jsFiles.sort (cmp);

        for (const auto& f : jsFiles)
        {
            // Display the path relative to the macros dir so subfolders
            // remain visible ("audio/normalize-all.js" etc.).
            choices.add (f.getRelativePathFrom (macroDir));
            values.add  (var (f.getFullPathName()));
        }
    }

    pathProp = new ChoicePropertyComponent (*pathValue, "Macro", choices, values);
    properties.add (pathProp);

    return properties;
}
