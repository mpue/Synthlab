//
//  EventListener.h
//  Synthlab
//
//  Created by Matthias Pueski on 05.04.18.
//

#ifndef EventListener_h
#define EventListener_h

#include "Event.h"



class EventListener {
    
public:
    virtual void eventReceived(Event* e) = 0;
    virtual ~EventListener(){};
};
#endif /* EventListener_h */
