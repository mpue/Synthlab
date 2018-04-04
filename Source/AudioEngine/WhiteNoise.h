/*
  ==============================================================================

    WhiteNoise.h
    Created: 3 Jun 2016 9:05:43pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#ifndef WHITENOISE_H_INCLUDED
#define WHITENOISE_H_INCLUDED

#include "Oszillator.h"
#include "Modulator.h"
#include "../JuceLibraryCode/JuceHeader.h"

class WhiteNoise : public Oszillator, public Modulator {
    
public:
    
    WhiteNoise(double sampleRate);
    ~WhiteNoise();
    
    virtual float process() override;
    virtual float getOutput() override;
    virtual void setFine(float fine) override;
    virtual float getFine() const override;
    
    virtual void reset() override;
    
    float fine;
    
private:
    ScopedPointer<Random> random;
    
};



#endif  // WHITENOISE_H_INCLUDED
