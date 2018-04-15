/*
  ==============================================================================

    Random.h
    Created: 13 Apr 2018 5:21:04pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#ifndef RANDOM_H_INCLUDED
#define RANDOM_H_INCLUDED


#include "Oszillator.h"
#include "Modulator.h"
#include "../JuceLibraryCode/JuceHeader.h"

namespace Synthlab {
    class Random : public Oszillator, public Modulator {
        
    public:
        
        Random(float sampleRate, int bufferSize);
        
        virtual float process() override;
        virtual float getOutput() override;
        virtual void setFrequency(double frequency) override;
        virtual void reset() override;
        virtual void setFine(float fine) override{};
        virtual float getFine() const override { return 0;};
    
    private:
        juce::Random random;
        float lastValue;
        int currentSample = 0;
        long currentTime = 0;
        int buffersize;
    };

    
    
}




#endif  // RANDOM_H_INCLUDED
