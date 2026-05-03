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
#include "UITools.h"

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

        // width = UITools::snap(width,this->raster,4);

        // Beim Links-Resize muss die RECHTE Kante stehenbleiben — also X aus
        // der vorherigen rechten Kante minus der geclampten Breite ableiten.
        // Die base class wuerde das machen; wir ueberschreiben checkBounds aber
        // komplett, also hier explizit.
        if (isStretchingLeft) {
            const int rightEdge = previousBounds.getRight();
            int newX = rightEdge - width;
            if (newX < 0) {
                newX = 0;
                width = rightEdge;
            }
            bounds.setX(newX);
            bounds.setWidth(width);
        } else {
            bounds.setWidth(width);
        }
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
    int maxWidth = 1000;
    
};




#endif  // DRAGCONSTRAINER_H_INCLUDED


#endif /* ResizeConstrainer_h */
