/*
  ==============================================================================

    ValueToEvent.cpp
    Created: 13 Oct 2025 6:15:51pm
    Author:  mpue

  ==============================================================================
*/

#include "ValueToEvent.h"
#include "Connection.h"

using namespace juce;

//==============================================================================
// Konstruktor / Destruktor
//------------------------------------------------------------------------------
ValueToEvent::ValueToEvent()
{
    setSize(120, 100);
	setName("ValueToEvent");
    editable = false;
    prefab = true;
    createProperties();
}

ValueToEvent::~ValueToEvent()
{
    delete channelValue;
    delete controllerValue;
    delete channelListener;
    delete controllerListener;
}

//==============================================================================
// Pins konfigurieren
//------------------------------------------------------------------------------
void ValueToEvent::configurePins()
{
    // VALUE-Eingang
    auto* pValueIn = new Pin(Pin::Type::VALUE);
    pValueIn->direction = Pin::Direction::IN;
    pValueIn->setName("V");
    addPin(Pin::Direction::IN, pValueIn);

    // EVENT-Ausgang
    auto* pEventOut = new Pin(Pin::Type::EVENT);
    pEventOut->direction = Pin::Direction::OUT;
    pEventOut->setName("E");
    addPin(Pin::Direction::OUT, pEventOut);
}

//==============================================================================
// Paint
//------------------------------------------------------------------------------
void ValueToEvent::paint(Graphics& g)
{
    Module::paint(g);

    // Modul-Beschriftung
    g.setColour(Colours::white);
    g.setFont(Font(10, Font::bold));
    g.drawText("V→E", 10, 40, getWidth() - 20, 20,
        Justification::centred);

    // Controller-Nummer anzeigen
    g.setFont(Font(9));
    g.drawText("CC" + String(controllerNum), 10, 60, getWidth() - 20, 15,
        Justification::centred);
}

//==============================================================================
// Process: Value-Input in Controller-Event umwandeln
//------------------------------------------------------------------------------
void ValueToEvent::process()
{
    if (pins.size() < 2) return;

    Pin* inputPin = pins.at(0);
    Pin* outputPin = pins.at(1);

    if (!inputPin || !outputPin) return;

	if (inputPin->getConnections().size() == 0) return;
    
    float currentValue = inputPin->getConnections().at(0)->getValue();

    // Nur bei Änderung ein Event senden
    if (currentValue != lastValue)
    {
        lastValue = currentValue;

        // Value in MIDI-Bereich (0-127) mappen
        int midiValue = mapToMidiRange(currentValue);

        // MIDI-Controller-Event erstellen
        MidiMessage msg = MidiMessage::controllerEvent(channel,
            controllerNum,
            midiValue);

        Event e("controller", Event::Type::CONTROLLER);
        e.setNumber(controllerNum);        
        e.setValue(midiValue);
        e.setMessage(msg);

        // Event an alle Verbindungen senden
        for (auto* conn : outputPin->getConnections())
        {
            conn->sendEvent(new Event(e));
        }
    }
}

//==============================================================================
// Hilfsfunktion: Value in MIDI-Bereich mappen
//------------------------------------------------------------------------------
int ValueToEvent::mapToMidiRange(float value)
{
    // Annahme: Value kommt im Bereich 0-127
    // Falls andere Bereiche nötig sind, hier anpassen
    int midiValue = juce::jlimit(0, 127, (int)std::round(value));
    return midiValue;
}

//==============================================================================
// Properties
//------------------------------------------------------------------------------
void ValueToEvent::createProperties()
{
    channelValue = new Value(1);
    controllerValue = new Value(1);

    channelListener = new ChannelListener(*channelValue, this);
    controllerListener = new ControllerListener(*controllerValue, this);
}

Array<PropertyComponent*>& ValueToEvent::getProperties()
{
    properties.clear();

    channelProp = new SliderPropertyComponent(*channelValue,
        "MIDI Channel",
        1, 16, 1, 1, true);

    controllerProp = new SliderPropertyComponent(*controllerValue,
        "CC Number",
        0, 127, 1, 1, true);

    properties.add(channelProp);
    properties.add(controllerProp);

    return properties;
}

//==============================================================================
// Setter
//------------------------------------------------------------------------------
void ValueToEvent::setChannel(int newChannel)
{
    channel = juce::jlimit(1, 16, newChannel);
    if (channelValue)
        channelValue->setValue(channel);
    repaint();
}

void ValueToEvent::setController(int newController)
{
    controllerNum = juce::jlimit(0, 127, newController);
    if (controllerValue)
        controllerValue->setValue(controllerNum);
    repaint();
}