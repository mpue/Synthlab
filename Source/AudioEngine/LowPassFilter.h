//
//  LowPassFilter.hpp
//  Trio
//
//  Created by Matthias Pueski on 16.11.16.
//
//

#ifndef LowPassFilter_hpp
#define LowPassFilter_hpp

#include <stdio.h>
#include "Filter.h"
#include "Modulator.h"
#include "ModTarget.h"

#include "../JuceLibraryCode/JuceHeader.h"

class LowPassFilter : public Filter, public ModTarget {
    
public:
    virtual void coefficients(float sampleRate, float frequency, float resonance) override;
    virtual void process(float *in, float *out,int numSamples);
    virtual void setModulator(Modulator* mod) override;
    virtual void setModAmount(float amount) override;
    
    LowPassFilter();
    virtual ~LowPassFilter();
    
private:
    juce::IIRFilter* filter1;
    juce::IIRFilter* filter2;
    Modulator* modulator;
    float frequency;
    float modAmount;
    float resonance;
    JUCE_LEAK_DETECTOR(LowPassFilter);

};


#endif /* LowPassFilter_hpp */

