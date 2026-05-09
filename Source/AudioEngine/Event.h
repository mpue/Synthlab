//
//  Event.hpp
//  Synthlab - App
//
//  Created by Matthias Pueski on 05.04.18.
//

#ifndef Event_hpp
#define Event_hpp

#include "../JuceLibraryCode/JuceHeader.h"

class Pin;   // forward — Event traegt optional einen Source-Pin-Verweis

class Event {

public:

	enum Type {
		GATE,
		NOTE,
		CONTROLLER,
		PITCH,
		CLOCK,
		CLOCK_START,
		CLOCK_STOP
	};

	Event();
	Event(Event* e);
	Event(juce::String name, Type type);
	~Event();

	juce::String getName();
	void setType(Type type);
	Type getType();
	int getValue();
	void setValue(int value);
	int getNote();
	void setNote(int note);
	int getNumber();
	void setNumber(int num);
	void setMessage(MidiMessage message);
	MidiMessage getMessage();

	// Optionaler Verweis auf den Pin der das Event versendet hat. Wird in
	// Pin::sendEvent gesetzt, damit ein Modul mit mehreren EVENT-IN-Pins
	// (z.B. Clk + Rst) im eventReceived-Callback unterscheiden kann.
	// Default nullptr fuer Events die nicht ueber Pin::sendEvent laufen.
	void setSource(Pin* p);
	Pin* getSource() const;
private:
	MidiMessage message;
	Type type;
	juce::String name;
	int number = 1;
	int value = 0;
	int note = 0;
	Pin* source = nullptr;
};


#endif /* Event_hpp */
