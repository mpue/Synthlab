/*
  ==============================================================================

    WaveTable.h
    Created: 25 Dec 2018 3:14:02pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#pragma once

#include <stdio.h>
#include "Oszillator.h"
#include "Modulator.h"

class WaveTable : public Oszillator , public Modulator {
    
    
public:
    WaveTable(float sampleRate, int buffersize);
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
    double value;
    bool on = false;
};

