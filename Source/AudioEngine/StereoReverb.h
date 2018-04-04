/*
  ==============================================================================

    StereoReverb.h
    Created: 4 Dec 2016 4:15:58pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#ifndef STEREOREVERB_H_INCLUDED
#define STEREOREVERB_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"

#include "StereoEffect.h"


class StereoReverb : public StereoEffect {
    
public:
    
    StereoReverb();
    ~StereoReverb();
    
    virtual void processStereo (float* const left, float* const right, const int numSamples) override;
    void setParameters (const juce::Reverb::Parameters& newParams);
    const juce::Reverb::Parameters& getParameters();
    void reset();
    
private:
    ScopedPointer<Reverb> reverb;
    
    JUCE_LEAK_DETECTOR(StereoReverb);
    
};




#endif  // STEREOREVERB_H_INCLUDED
