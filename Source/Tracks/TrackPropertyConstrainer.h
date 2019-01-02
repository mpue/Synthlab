/*
  ==============================================================================

    TrackPropertyConstrainer.h
    Created: 17 Dec 2016 4:24:31pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#ifndef TRACKPROPERTYCONSTRAINER_H_INCLUDED
#define TRACKPROPERTYCONSTRAINER_H_INCLUDED


#include "../JuceLibraryCode/JuceHeader.h"

class TrackPropertyConstrainer : public ComponentBoundsConstrainer {
    
public:
    
    TrackPropertyConstrainer(int minHeight, int maxHeight) {
        this->minHeight = minHeight;
        this->maxHeight = maxHeight;
    }
    
    virtual void checkBounds (Rectangle<int>& bounds,
                              const Rectangle<int>& previousBounds,
                              const Rectangle<int>& limits,
                              bool isStretchingTop,
                              bool isStretchingLeft,
                              bool isStretchingBottom,
                              bool isStretchingRight) override {
        
        if (bounds.getHeight() > maxHeight) {
            bounds.setHeight(maxHeight);
        }
        if (bounds.getHeight() < minHeight) {
            bounds.setHeight(minHeight);
        }
        
    }
    
private:
    int maxHeight;
    int minHeight;
    
};

#endif  // TRACKPROPERTYCONSTRAINER_H_INCLUDED
