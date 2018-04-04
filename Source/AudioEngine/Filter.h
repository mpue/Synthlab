/*
  ==============================================================================

    Filter.h
    Created: 9 Jun 2016 6:32:17pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#ifndef FILTER_H_INCLUDED
#define FILTER_H_INCLUDED

#include "Modulator.h"

class Filter {
    
public:
    
    virtual ~Filter();
    
    virtual void coefficients(float frequency, float resonance) = 0;
    
    
};



#endif  // FILTER_H_INCLUDED
