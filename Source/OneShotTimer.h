//
//  OneShotTimer.h
//  Synthlab
//
//  Created by Matthias Pueski on 17.04.18.
//

#ifndef OneShotTimer_h
#define OneShotTimer_h

#include "../JuceLibraryCode/JuceHeader.h"

typedef void(*Callback) (void);

class OneShotTimer : public juce::Timer {
    
public:
    
    OneShotTimer(int milliseconds,Callback c) {
        shot = milliseconds;
        this->callback = c;
    }
    
    void start() {
        startTimer(shot);
    }
    
    virtual void timerCallback() override {
        stopTimer();
        callback();
    }
    
   
    Callback callback;
    
private:
    
    int shot;
    
};


#endif /* OneShotTimer_h */
