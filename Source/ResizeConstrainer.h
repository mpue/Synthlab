//
//  ResizeConstrainer.h
//  WaveX
//
//  Created by Matthias Pueski on 30.12.16.
//
//

#ifndef ResizeConstrainer_h
#define ResizeConstrainer_h
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
#include <math.h>

class ResizeConstrainer : public ComponentBoundsConstrainer {
    
public:
    
    ResizeConstrainer() {
    }
    
    ResizeConstrainer(int raster) {
        this->raster = raster;
    }
    
    virtual void checkBounds (Rectangle<int>& bounds,
                              const Rectangle<int>& previousBounds,
                              const Rectangle<int>& limits,
                              bool isStretchingTop,
                              bool isStretchingLeft,
                              bool isStretchingBottom,
                              bool isStretchingRight) override {
        
        int width = bounds.getWidth();
        
        if (width > maxWidth) {
            width = maxWidth;
        }
        
        width = snap(width,this->raster);
        
        bounds.setWidth(width);
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
    
    void setMaxWidth(int maxWidth) {
        this->maxWidth = maxWidth;
    }
    
    void setTolerance(int tolerance) {
        this->tolerance = tolerance;
    }

    
private:
    int raster = 1;
    int tolerance = 4;
    int maxWidth = 0;
    
};




#endif  // DRAGCONSTRAINER_H_INCLUDED


#endif /* ResizeConstrainer_h */
