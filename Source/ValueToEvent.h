/*
  ==============================================================================

    ValueToEvent.h
    Created: 13 Oct 2025 6:15:51pm
    Author:  mpue

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"

//==============================================================================
/**
 * ValueToEvent wandelt VALUE-Eingänge in MIDI-Controller-Events um.
 * Damit kann der Knob als MIDI-Controller verwendet werden.
 */
class ValueToEvent : public Module
{
public:
    ValueToEvent();
    ~ValueToEvent() override;

    // JUCE
    void paint(juce::Graphics&) override;
    void configurePins() override;

    // Verarbeitung
    void process() override;

    // Properties
    juce::Array<juce::PropertyComponent*>& getProperties() override;
    void createProperties() override;

    void setChannel(int newChannel);
    void setController(int newController);

    int getChannel() const { return channel; }
    int getController() const { return controllerNum; }

    juce::String getCategory() override { return "MIDI"; }

private:
    // ----------------------------------------------------------------
    // GUI-Property-Listener
    struct ChannelListener : juce::Value::Listener
    {
        ChannelListener(juce::Value& v, ValueToEvent* owner)
            : owner(owner), value(v)
        {
            value.addListener(this);
        }

        void valueChanged(juce::Value&) override
        {
            owner->setChannel(value.getValue());
        }

        ValueToEvent* owner;
        juce::Value value;
    };

    struct ControllerListener : juce::Value::Listener
    {
        ControllerListener(juce::Value& v, ValueToEvent* owner)
            : owner(owner), value(v)
        {
            value.addListener(this);
        }

        void valueChanged(juce::Value&) override
        {
            owner->setController(value.getValue());
        }

        ValueToEvent* owner;
        juce::Value value;
    };

    // ----------------------------------------------------------------
    // Hilfsfunktion: Value in MIDI-Bereich mappen
    int mapToMidiRange(float value);

    // ----------------------------------------------------------------
    // Daten
    int channel{ 1 };
    int controllerNum{ 1 };
    float lastValue{ -1.0f };  // Für Change-Detection

    juce::Value* channelValue{ nullptr };
    juce::Value* controllerValue{ nullptr };

    ChannelListener* channelListener{ nullptr };
    ControllerListener* controllerListener{ nullptr };

    juce::PropertyComponent* channelProp{ nullptr };
    juce::PropertyComponent* controllerProp{ nullptr };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ValueToEvent)
};