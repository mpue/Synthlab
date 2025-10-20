/*
  ==============================================================================

	AlignVerticalAction.h
	Created: 20 Oct 2025 7:57:58pm
	Author:  mpue

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
class SynthEditor;

class AlignVerticalAction : public juce::UndoableAction {

public:

	AlignVerticalAction(SynthEditor* editor) {
		this->editor = editor;
	}
	~AlignVerticalAction() override = default;

	bool perform() override;
	bool undo() override;


private:

	SynthEditor* editor;
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AlignVerticalAction)
};