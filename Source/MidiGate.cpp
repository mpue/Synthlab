/*
  ==============================================================================
   MidiGate.cpp – polyphon-sicher (2025-07-09)
  ==============================================================================
*/
#include "MidiGate.h"
#include "Connection.h"

using namespace juce;

//==============================================================================
// Konstruktor / Destruktor
//------------------------------------------------------------------------------
MidiGate::MidiGate()
{
    setSize(120, 140);
    nameLabel->setJustificationType(Justification::left);
    nameLabel->setTopLeftPosition(2, 2);
    editable = false;  prefab = true;
    createProperties();
}

MidiGate::~MidiGate()
{
    delete channelValue;
    delete channelListener;
}

//==============================================================================
// Pins
void MidiGate::configurePins()
{
    // VALUE-Ausgang (Gate-Spannung / Velocity)
    auto* pValue = new Pin(Pin::Type::VALUE);
    pValue->direction = Pin::Direction::OUT;
    pValue->setName("G");
    addPin(Pin::Direction::OUT, pValue);

    // EVENT-Ausgang (Note-Events)
    auto* pEvent = new Pin(Pin::Type::EVENT);
    pEvent->direction = Pin::Direction::OUT;
    pEvent->setName("E");
    addPin(Pin::Direction::OUT, pEvent);
}

//==============================================================================
void MidiGate::paint(Graphics& g)
{
    Module::paint(g);

    // Icon zeichnen
    g.drawImageAt(ImageCache::getFromMemory(BinaryData::midiin_png,
        BinaryData::midiin_pngSize),
        25, 40);

    // LED zeichnen (kleiner Kreis oben rechts)
    const int ledSize = 10;
    const int x = getWidth() - ledSize - 4;
    const int y = getHeight() - ledSize -4;

    g.setColour(gateActive ? Colours::yellow : Colours::darkgrey);
    g.fillEllipse((float)x, (float)y, (float)ledSize, (float)ledSize);
    g.setColour(Colours::black.withAlpha(0.3f));
    g.drawEllipse((float)x, (float)y, (float)ledSize, (float)ledSize, 1.0f);
}

//==============================================================================
// Gate-Logik  ---------------------------------------------------------------

void MidiGate::gateOn(int velocity, int note)
{
    activeNotes.insert(note);
    if (!gateActive)
    {
        gateActive = true;
        repaint(); // LED einschalten
    }

    pins.at(0)->setValue(velocity);
    pins.at(0)->setNote(note);

    Event e("gate on", Event::Type::GATE);
    e.setValue(velocity);
    e.setNote(note);
    e.setMessage(MidiMessage::noteOn(channel, note, (uint8)velocity));

    for (auto* conn : pins.at(1)->getConnections())
        conn->sendEvent(new Event(e));
}

void MidiGate::gateOff(int note)
{
    activeNotes.erase(note);

    Event e("gate off", Event::Type::GATE);
    e.setValue(0);
    e.setNote(note);
    e.setMessage(MidiMessage::noteOff(channel, note));

    for (auto* conn : pins.at(1)->getConnections())
        conn->sendEvent(new Event(e));

    if (activeNotes.empty() && gateActive)
    {
        pins.at(0)->setValue(0);
        gateActive = false;
        repaint(); // LED ausschalten
    }
}

//==============================================================================
// Event-Passthrough (nur wenn Kanal passt)
void MidiGate::eventReceived(Event* e)
{
    if (!e) return;

    int msgChannel = 1; // Fallback-Kanal
    const auto& msg = e->getMessage();

    if (msg.isNoteOn() || msg.isNoteOff() || msg.isController() || msg.isPitchWheel())
        msgChannel = msg.getChannel();

    if (msgChannel == channel)
        for (auto* conn : pins.at(1)->getConnections())
            conn->sendEvent(new Event(*e));

    delete e;
}

//==============================================================================
// Properties
void MidiGate::createProperties()
{
    channelValue = new Value(1);
    channelListener = new ChannelListener(*channelValue, this);
}

Array<PropertyComponent*>& MidiGate::getProperties()
{
    properties.clear();
    channelProp = new SliderPropertyComponent(*channelValue,
        "channel",
        1, 16, 1, 1, true);
    properties.add(channelProp);
    return properties;
}

//==============================================================================
// Sonstiges
void MidiGate::setChannel(int newChannel)
{
    channel = jmax(1, newChannel);
    if (channelValue) channelValue->setValue(channel);
}
