/*
  ==============================================================================

	DefaultUndoManager.h
	Created: 18 Oct 2025 6:24:52pm
	Author:  mpue

  ==============================================================================
*/

#pragma once

#include <stack>

#include "../JuceLibraryCode/JuceHeader.h"
#include "Actions/DefaultUndoableAction.h"

class DefaultUndoManager
{

public:

	DefaultUndoManager()
	{
	}
	~DefaultUndoManager()
	{
	}

	bool canUndo() const
	{
		return !undoStack.empty();
	}

	bool canRedo() const
	{
		return !redoStack.empty();
	}

	int getNumActions() const
	{
		return static_cast<int>(undoStack.size());
	}

	juce::StringArray getUndoStackDescriptions() const
	{
		juce::StringArray descriptions;
		std::stack<DefaultUndoableAction*> tempStack = undoStack;
		while (!tempStack.empty())
		{
			DefaultUndoableAction* action = tempStack.top();
			tempStack.pop();
			descriptions.add(action->getDescription());
		}
		return descriptions;
	}

	// undoes the last action, and removes it from the undo stack
	void undo()
	{
		if (!undoStack.empty())
		{
			DefaultUndoableAction* action = undoStack.top();
			if (action->undo())
			{
				undoStack.pop();
				redoStack.push(action);
			}
		}
	}

	void clear()
	{
		while (!undoStack.empty())
		{
			DefaultUndoableAction* action = undoStack.top();
			undoStack.pop();
			delete action;
		}
	}

	void redo()
	{
		if (!redoStack.empty())
		{
			DefaultUndoableAction* action = redoStack.top();
			action->perform();
			redoStack.pop();
			undoStack.push(action);
		}

	}

	void perform(DefaultUndoableAction* action) {
		if (action->perform())
		{
			undoStack.push(action);
		}
		else
		{
			delete action;
		}
	}

private:

	std::stack<DefaultUndoableAction*> undoStack;
	std::stack<DefaultUndoableAction*> redoStack;

	DefaultUndoableAction* lastAction = nullptr;

};
