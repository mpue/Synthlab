//
//  Sine.hpp
//  Trio
//
//  Created by Matthias Pueski on 20.11.16.
//
//

#ifndef Sine_hpp
#define Sine_hpp

#include <stdio.h>
#include "Oszillator.h"
#include "Modulator.h"

class Sine : public Oszillator , public Modulator {
    
    
public:
    Sine(float sampleRate, int buffersize);
    virtual float process() override;
    virtual void setFine(float fine) override;
    virtual float getFine() const override;
    virtual void setFrequency(double frequency) override;
    virtual float getOutput() override;
    virtual void reset() override;
    
private:
    float fine;
    double step;
    double phase;
    float value;
};

#endif /* Sine_hpp */
