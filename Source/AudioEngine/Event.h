//
//  Event.hpp
//  Synthlab - App
//
//  Created by Matthias Pueski on 05.04.18.
//

#ifndef Event_hpp
#define Event_hpp

#include "../JuceLibraryCode/JuceHeader.h"

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
    Type getType();
    int getValue();
    void setValue(int value);
    int getNote();
    void setNote(int note);
    int getNumber();
    void setNumber(int num);
private:
    
    Type type;
    juce::String name;
    int number = 1;
    int value = 0;
    int note = 0;
};


#endif /* Event_hpp */
