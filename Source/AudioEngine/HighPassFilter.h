//
//  LowPassFilter.hpp
//  Trio
//
//  Created by Matthias Pueski on 16.11.16.
//
//

#ifndef HighPassFilter_hpp
#define HighPassFilter_hpp

#include <stdio.h>
#include "Filter.h"
#include "Modulator.h"
#include "ModTarget.h"

#include "../JuceLibraryCode/JuceHeader.h"

class HighPassFilter : public Filter, public ModTarget {
    
public:
    virtual void coefficients(float frequency, float resonance) override;
    virtual void process(float *in, float *out,int numSamples);
    virtual void setModulator(Modulator* mod) override;
    virtual void setModAmount(float amount) override;
    
    HighPassFilter();
    virtual ~HighPassFilter();
    
private:
    ScopedPointer<IIRFilter> filter1;
    ScopedPointer<IIRFilter> filter2;
    Modulator* modulator;
    float frequency;
    float modAmount;
    float resonance;
    JUCE_LEAK_DETECTOR(HighPassFilter);
    
};


#endif /* HighPassFilter_hpp */

