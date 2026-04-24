/*
  ==============================================================================

    RadioGroupModule.h
    Created: 2026
    Author:  Matthias Pueski

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"

//==============================================================================
/*
    A configurable array of toggle buttons acting as a radio group.
    Exactly one button is active at a time.

    Layers:
      MODULES / ALL  – compact module view with only pins visible
      GUI            – shows the toggle button array

    Pins:
      0 - VALUE OUT  (index of the active button, 0-based)
*/
class RadioGroupModule : public Module,
                         public juce::Button::Listener
{
public:
    RadioGroupModule(int bufferSize);
    ~RadioGroupModule();

    void paint(juce::Graphics& g) override;
    void process() override;
    void configurePins() override;
    void setCurrentLayer(Layer layer) override;

    Layer getLayer() override { return Layer::GUI; }

    juce::String getCategory() override { return "Controls"; }

    void buttonClicked(juce::Button* b) override;

    void setNumButtons(int n);
    int  getNumButtons() const { return numButtons; }
    int  getSelection()  const { return selection; }
    void setSelection(int s);

private:
    void rebuildButtons();
    void updateButtonStates();
    void layoutButtons();

    int numButtons = 3;
    int selection  = 0;

    juce::OwnedArray<juce::TextButton> buttons;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RadioGroupModule)
};
