/*
  ==============================================================================

    StereoDelay.h
    Created: 4 Dec 2016 11:19:25am
    Author:  Matthias Pueski

  ==============================================================================
*/

#ifndef STEREODELAY_H_INCLUDED
#define STEREODELAY_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"
#include "StereoEffect.h"
#include "BasicDelayLine.h"


class StereoDelay : public StereoEffect {
    
public:
    StereoDelay();
    ~StereoDelay();
    
    enum Channel {
        LEFT,
        RIGHT
    };
    
    void processStereo (float* const left, float* const right, const int numSamples);

    void setDelay(Channel channel, float time);
    void setFeedback(Channel channel, float fb);
    void setMix(Channel channel, float mix);
    void setByPass(bool bp);

    void resetDelay();
    
private:
    juce::ScopedPointer<BasicDelayLine> delayLeft;
    juce::ScopedPointer<BasicDelayLine> delayRight;
    
    JUCE_LEAK_DETECTOR(StereoDelay);
    
};



#endif  // STEREODELAY_H_INCLUDED
