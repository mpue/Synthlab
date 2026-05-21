/*
  ==============================================================================

    DeleteNoteAction.cpp
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "DeleteNoteAction.h"
#include "../Tracks/MidiRegion.h"

DeleteNoteAction::DeleteNoteAction (MidiRegion* r,
                                    const juce::MidiMessage& noteOn,
                                    const juce::MidiMessage& noteOff)
    : region (r), onCopy (noteOn), offCopy (noteOff)
{
}

bool DeleteNoteAction::perform()
{
    if (region == nullptr) return false;

    auto& messages = region->getMidiMessageList();

    auto removeByValue = [&] (const juce::MidiMessage& target, bool wantNoteOn)
    {
        auto it = std::find_if (messages.begin(), messages.end(),
            [&] (juce::MidiMessage* m)
            {
                return m != nullptr
                    && (wantNoteOn ? m->isNoteOn() : m->isNoteOff())
                    && m->getNoteNumber() == target.getNoteNumber()
                    && m->getChannel()    == target.getChannel()
                    && m->getTimeStamp()  == target.getTimeStamp();
            });
        if (it != messages.end()) { delete *it; messages.erase (it); }
    };

    removeByValue (onCopy,  true);
    removeByValue (offCopy, false);

    region->notifyDirectEdit();
    return true;
}

bool DeleteNoteAction::undo()
{
    if (region == nullptr) return false;

    auto* newOn  = new juce::MidiMessage (onCopy);
    auto* newOff = new juce::MidiMessage (offCopy);

    auto& messages = region->getMidiMessageList();
    messages.push_back (newOn);
    messages.push_back (newOff);

    region->notifyDirectEdit();
    return true;
}
