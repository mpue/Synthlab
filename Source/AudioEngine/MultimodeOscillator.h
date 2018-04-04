//
//  MultimodeOscillator.hpp
//  Trio
//
//  Created by Matthias Pueski on 20.11.16.
//
//

#ifndef MultimodeOscillator_hpp
#define MultimodeOscillator_hpp

#include <stdio.h>

#include "Oszillator.h"
#include "Sine.h"
#include "Sawtooth.h"
#include "Pulse.h"
#include "WhiteNoise.h"
#include "ModTarget.h"

class MultimodeOscillator : public Oszillator, public Modulator, public ModTarget {
    
    
public:
    MultimodeOscillator(float sampleRate);
    ~MultimodeOscillator();
    virtual float process() override;
    virtual void setFine(float fine) override;
    virtual float getFine() const override;
    virtual void setFrequency(double frequency) override;
    virtual void setVolume(float volume) override;
    virtual float getOutput() override;
    virtual void setModulator(Modulator* mod) override;
    virtual void setModAmount(float amount) override;
    virtual void reset() override;
    virtual void setSlave(Oszillator* slave) override;
    virtual void setSync(bool sync) override;
    
private:
    float fine;
    Sawtooth* saw;
    Sine* sine;
    Pulse* pulse;
    WhiteNoise* noise;
    float modAmount;
    Modulator* modulator;
};

#endif /* MultimodeOscillator_hpp */
