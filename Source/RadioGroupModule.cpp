/*
  ==============================================================================

    RadioGroupModule.cpp
    Created: 2026
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "RadioGroupModule.h"
#include "Connection.h"

using juce::TextButton;
using juce::dontSendNotification;

// Compact size used in MODULES / ALL layer
static constexpr int kCompactW = 90;
static constexpr int kCompactH = 40;

// Per-button metrics in GUI layer
static constexpr int kBtnH    = 26;
static constexpr int kBtnGap  = 4;
static constexpr int kPadX    = 6;
static constexpr int kPadY    = 8;
static constexpr int kGuiW    = 120;

//==============================================================================
RadioGroupModule::RadioGroupModule(int bufferSize)
{
    setSize(kCompactW, kCompactH);
    setName("Radio group");
    editable = false;
    prefab   = true;
    this->buffersize = bufferSize;

    rebuildButtons();
    setInterceptsMouseClicks(false, false);
}

RadioGroupModule::~RadioGroupModule()
{
}

void RadioGroupModule::configurePins()
{
    Pin* out = new Pin(Pin::Type::VALUE);
    out->direction = Pin::Direction::OUT;
    out->setName("Idx");
    addPin(Pin::Direction::OUT, out);
}

//==============================================================================
void RadioGroupModule::rebuildButtons()
{
    // Remove and delete existing buttons
    for (auto* b : buttons)
        removeChildComponent(b);
    buttons.clear();

    for (int i = 0; i < numButtons; ++i)
    {
        auto* btn = new TextButton(juce::String(i));
        btn->setClickingTogglesState(true);
        btn->setToggleState(i == selection, dontSendNotification);
        btn->addListener(this);
        addChildComponent(btn);   // invisible until GUI layer is active
        buttons.add(btn);
    }
}

void RadioGroupModule::updateButtonStates()
{
    for (int i = 0; i < buttons.size(); ++i)
        buttons[i]->setToggleState(i == selection, dontSendNotification);
}

void RadioGroupModule::layoutButtons()
{
    int y = kPadY;
    for (auto* b : buttons)
    {
        b->setBounds(kPadX, y, getWidth() - 2 * kPadX, kBtnH);
        y += kBtnH + kBtnGap;
    }
}

//==============================================================================
void RadioGroupModule::setNumButtons(int n)
{
    numButtons = juce::jmax(1, n);
    if (selection >= numButtons)
        selection = 0;
    rebuildButtons();
    setCurrentLayer(currentLayer);   // re-apply layout / visibility
}

void RadioGroupModule::setSelection(int s)
{
    selection = juce::jlimit(0, numButtons - 1, s);
    updateButtonStates();
}

//==============================================================================
void RadioGroupModule::buttonClicked(juce::Button* b)
{
    for (int i = 0; i < buttons.size(); ++i)
    {
        if (buttons[i] == b)
        {
            selection = i;
            break;
        }
    }
    // Enforce radio behaviour: exactly one button active
    updateButtonStates();
}

//==============================================================================
void RadioGroupModule::setCurrentLayer(Layer layer)
{
    Module::setCurrentLayer(layer);

    const bool guiActive = (layer == Layer::GUI);

    if (!guiActive)
    {
        // MODULES / ALL layer: compact schematic box, buttons hidden
        setSize(kCompactW, kCompactH);
        for (auto* b : buttons)
            b->setVisible(false);
        setInterceptsMouseClicks(false, false);
    }
    else
    {
        // GUI layer: show buttons stacked vertically
        int h = kPadY + numButtons * (kBtnH + kBtnGap) - kBtnGap + kPadY;
        setSize(kGuiW, h);
        layoutButtons();
        for (auto* b : buttons)
            b->setVisible(true);
        setInterceptsMouseClicks(false, true);  // children (buttons) must receive clicks
    }
}

//==============================================================================
void RadioGroupModule::process()
{
    if (!pins.empty())
        pins.at(0)->setValue(static_cast<float>(selection));
}

void RadioGroupModule::paint(juce::Graphics& g)
{
    Module::paint(g);
}
