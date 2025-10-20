/*
  ==============================================================================

    DefaultUndoableAction.h
    Created: 18 Oct 2025 9:20:44pm
    Author:  mpue

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class DefaultUndoableAction
{

public:
    DefaultUndoableAction() {}
    virtual ~DefaultUndoableAction() {}
    virtual bool perform() = 0;
    virtual bool undo() = 0;
    virtual bool redo() { return perform(); }
    virtual juce::String getDescription() const { return ""; }


};