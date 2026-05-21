/*
  ==============================================================================

    DeleteNoteAction.h
    Author:  Matthias Pueski

    Undoable removal of a note-on/note-off pair from a MidiRegion. Inverse of
    AddNoteAction — perform() erases by value, undo() recreates from value
    copies. Caller is responsible for picking the correct pair (see the JS
    bridge in JavaScriptEditorComponent.cpp which uses FIFO pairing matching
    PianoRollComponent::findNoteOffForNoteOn).

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class MidiRegion;

class DeleteNoteAction : public juce::UndoableAction
{
public:
    DeleteNoteAction (MidiRegion* region,
                      const juce::MidiMessage& noteOn,
                      const juce::MidiMessage& noteOff);

    bool perform() override;
    bool undo() override;

private:
    MidiRegion* region;
    juce::MidiMessage onCopy;
    juce::MidiMessage offCopy;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DeleteNoteAction)
};
