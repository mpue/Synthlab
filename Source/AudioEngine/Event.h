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
    
    static enum Type {
        GATE
    };
    
    Event();
    Event(Event* e);
    Event(String name, Type type);
    ~Event();
    
  String getName();
    Type getType();
    
private:
    
    Type type;
    String name;
    
};


#endif /* Event_hpp */
