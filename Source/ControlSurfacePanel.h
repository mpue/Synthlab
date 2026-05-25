#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"
#include "EditorListener.h"

class SynthEditor;

// Second view onto the main SynthEditor's module hierarchy: shows only
// controls flagged displayAtRootLevel (the "Controls only"-layer set), grouped
// by their immediate parent module. Built as a SHADOW tree of cloned control
// modules so the originals remain attached to the main editor (a JUCE
// Component can only have one parent).
//
// Sync strategy: for control modules that expose a juce::Slider (Knob,
// SliderModule, ...), the shadow's Slider points its ValueObject at the
// source's via juce::Value::referTo — both sliders share the same Value and
// every drag stays in sync without manual listener bookkeeping. Module types
// without a Slider get a placeholder label (clickable controls of unsupported
// types: TODO add per-type bindings as needed).
class ControlSurfacePanel : public juce::Component,
                            public juce::Button::Listener,
                            public EditorListener
{
public:
    ControlSurfacePanel();
    ~ControlSurfacePanel() override;

    // Non-owning. The editor is the source of truth; we hold a back-pointer
    // for rebuild() to walk its module tree. Pass nullptr to detach.
    void setEditor (SynthEditor* editor);

    // Tear down the shadow tree and rebuild it from the source editor's root.
    // Cheap enough to call on file-load or user request; do not call from
    // audio threads (allocates juce::Component).
    void rebuild();

    void paint (juce::Graphics& g) override;
    void resized() override;
    void buttonClicked (juce::Button* b) override;

    // EditorListener — required to break Value-bindings before the editor
    // destroys its source modules. The other callbacks are no-ops here.
    void beforeCleanUp() override;
    void selectionChanged (juce::Component*) override {}
    void fileChanged (juce::String) override {}
    void dirtyChanged (bool) override {}

private:
    struct ShadowEntry {
        // Cloned control module (we own it).
        std::unique_ptr<Module> shadow;
        // Source module in the editor (non-owning, raw). Becomes dangling if
        // the user deletes the source between two rebuild() calls — caller is
        // responsible for invoking rebuild() after structural mutations.
        Module* source = nullptr;
    };

    void clearShadow();
    void bindSliderPair (Module* source, Module* shadow);
    void copyConfig (Module* source, Module* shadow);

    SynthEditor* editor = nullptr;
    juce::OwnedArray<juce::GroupComponent> groups;
    std::vector<ShadowEntry> entries;
    std::unique_ptr<juce::TextButton> refreshButton;

    static constexpr int kHeaderHeight = 32;
};
