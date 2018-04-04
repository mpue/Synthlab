//
//  MultimodeFilter.hpp
//  Trio
//
//  Created by Matthias Pueski on 01.12.16.
//
//

#ifndef MultimodeFilter_hpp
#define MultimodeFilter_hpp

#include <stdio.h>
#include "Filter.h"
#include "LowPassFilter.h"
#include "HighPassFilter.h"
#include "StereoEffect.h"
#include "ModTarget.h"

#include "../JuceLibraryCode/JuceHeader.h"

class MultimodeFilter : public Filter, public StereoEffect, public ModTarget {

public:
    enum Mode {
        HIGHPASS,
        LOWPASS
    };
    
    MultimodeFilter();
    virtual ~MultimodeFilter();
    
    virtual void coefficients(float frequency, float resonance) override;
    virtual void processStereo(float *const left, float *const right, const int numSamples) override;
    virtual void setModulator(Modulator* mod) override;
    virtual void setModAmount(float amount) override;
    void setMode(Mode mode);

private:
    
    ScopedPointer<LowPassFilter> lowPassLeft;
    ScopedPointer<HighPassFilter> highPassLeft;
    
    ScopedPointer<LowPassFilter> lowPassRight;
    ScopedPointer<HighPassFilter> highPassRight;
    
    Mode mode;
    JUCE_LEAK_DETECTOR(MultimodeFilter);
    
};

#endif /* MultimodeFilter_hpp */