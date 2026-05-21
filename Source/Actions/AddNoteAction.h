/*
  ==============================================================================

    AddNoteAction.h
    Author:  Matthias Pueski

    Undoable insertion of a note-on/note-off pair into a MidiRegion. Stores
    value-copies of both MidiMessages — perform() allocates fresh heap copies
    and pushes them; undo() locates them by value (pitch + channel + timestamp)
    and erases. Same pattern as PianoRollComponent::deleteNotePair's undo
    closures, so we never hold the heap pointer between perform/undo cycles.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class MidiRegion;

class AddNoteAction : public juce::UndoableAction
{
public:
    AddNoteAction (MidiRegion* region,
                   const juce::MidiMessage& noteOn,
                   const juce::MidiMessage& noteOff);

    bool perform() override;
    bool undo() override;

private:
    MidiRegion* region;
    juce::MidiMessage onCopy;
    juce::MidiMessage offCopy;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AddNoteAction)
};
