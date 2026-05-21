/*
  ==============================================================================

    TriggerScriptModule.h
    Author:  Matthias Pueski

    Single-input EVENT module: receives a Gate signal and fires a user-
    selected JS macro (from ~/.Synthlab/macros/) on every gate-on rising
    edge. Bridges the modular signal world with the JS scripting layer.

    The macro is executed via JavaScriptEditorComponent::runFile through
    the existing Duktape pipeline — output and errors land in the JS
    editor's output panel. Execution is dispatched via
    MessageManager::callAsync because gate events fire on the audio thread
    and the script must not run there (allocations, modal dialogs, etc.).

    The macro is selected from a ChoicePropertyComponent listing all .js
    files in ~/.Synthlab/macros/ (rebuilt on each module-selection so newly
    dropped files appear immediately). Empty path = no macro bound = gate
    events ignored.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"

class TriggerScriptModule : public Module
{
public:
    TriggerScriptModule();
    ~TriggerScriptModule() override;

    void paint (juce::Graphics& g) override;

    void configurePins() override;

    juce::String getCategory() override { return "MIDI"; }

    void eventReceived (Event* e) override;

    // ModuleUtils save/load round-trip this via the "macroPath" XML
    // attribute. Empty string = no macro bound.
    void          setMacroPath (const juce::String& p);
    juce::String  getMacroPath() const { return macroPath; }

    juce::Array<juce::PropertyComponent*>& getProperties() override;
    void createProperties() override;

private:
    juce::String macroPath;
    Pin*         gatePin = nullptr;

    juce::Value*             pathValue = nullptr;
    juce::PropertyComponent* pathProp  = nullptr;

    struct PathListener : juce::Value::Listener
    {
        PathListener (juce::Value& v, TriggerScriptModule* m)
            : module (m), value (v) { value.addListener (this); }
        void valueChanged (juce::Value& v) override {
            module->setMacroPath (v.toString());
        }
        TriggerScriptModule* module;
        juce::Value value;
    };
    PathListener* pathListener = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TriggerScriptModule)
};
