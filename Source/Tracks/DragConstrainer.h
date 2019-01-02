/*
  ==============================================================================

    DragConstrainer.h
    Created: 16 Dec 2016 5:55:25pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#ifndef DRAGCONSTRAINER_H_INCLUDED
#define DRAGCONSTRAINER_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"
#include <iostream>
#include <math.h>

class DragConstrainer : public ComponentBoundsConstrainer {

public:
    
    DragConstrainer() {
    }
    
    DragConstrainer(int raster) {
        this->raster = raster;
    }
    
    virtual void checkBounds (Rectangle<int>& bounds,
                              const Rectangle<int>& previousBounds,
                              const Rectangle<int>& limits,
                              bool isStretchingTop,
                              bool isStretchingLeft,
                              bool isStretchingBottom,
                              bool isStretchingRight) override {
        
        if (bounds.getY() != previousBounds.getY()) {
            bounds.setY(previousBounds.getY());
        }
        if (bounds.getX() < 0) {
            bounds.setX(0);
        }
        if (bounds.getX() + bounds.getWidth() > maxX) {

            bounds.setX(maxX - bounds.getWidth());
        }
        bounds.setX(snap(bounds.getX(),this->raster));
    }
    
    double snap(double location, double raster) {
        
        int toleranceWindow = (raster / tolerance);
        
        if (location > 0) {
            
            
            if ((fmod(location,raster )) > toleranceWindow) {
                location = location + (raster - (fmod(location,raster)));
            }
            else {
                location = location - (fmod(location,raster));
            }
        }
        else {
            if ((fmod(location,raster)) < toleranceWindow) {
                location = location + (raster - (fmod(location,raster))) - raster;
            }
            else {
                location = location - (fmod(location, raster)) - raster;
            }
        }
        
        return location;
    }
    
    void setRaster(int raster) {
        this->raster = raster;
    }
    
	void setTolerance(int tolerance) {
		this->tolerance = tolerance;			
	}
    
    void setMaxX(int maxX) {
        this->maxX = maxX;
    }
    
private:
    int raster = 1;
	int tolerance = 4;
    int maxX = 0;
    
};




#endif  // DRAGCONSTRAINER_H_INCLUDED
