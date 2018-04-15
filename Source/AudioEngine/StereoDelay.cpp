/*
  ==============================================================================

    StereoDelay.cpp
    Created: 4 Dec 2016 11:19:25am
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "StereoDelay.h"

StereoDelay::StereoDelay() {
    delayLeft = new BasicDelayLine();
    delayLeft->setMix(0.5);
    delayLeft->setDelay(500);
    delayLeft->setFeedback(0.5);
    delayLeft->setUseExternalFeedback(false);
    
    delayRight = new BasicDelayLine();
    delayRight->setMix(0.5);
    delayRight->setDelay(375);
    delayRight->setFeedback(0.5);
    delayRight->setUseExternalFeedback(false);
}

StereoDelay::~StereoDelay() {
    delayRight = nullptr;
    delayLeft = nullptr;
}

void StereoDelay::processStereo(float *const left, float *const right, const int numSamples) {
    if (this->enabled) {
        for (int sample = 0; sample < numSamples; ++sample) {
            if (delayLeft != nullptr)
                left[sample] = delayLeft->next(left[sample]);
            if (delayRight != nullptr)
                right[sample] = delayRight->next(right[sample]);
        }
    }
}

void StereoDelay::setDelay(StereoDelay::Channel channel, float time) {

	// Logger::getCurrentLogger()->writeToLog("Setting delayTime for channel " + String(channel) + " to " + String(time) + "ms");

    if (channel == LEFT) {
        if (delayLeft != nullptr)
            delayLeft->setDelay(time);
    }
    else if (channel == RIGHT) {
        if (delayRight != nullptr)
            delayRight->setDelay(time);
    }
}

void StereoDelay::setMix(StereoDelay::Channel channel, float mix) {
    if (channel == LEFT) {
        if (delayLeft != nullptr)
            delayLeft->setMix(mix);
    }
    else if (channel == RIGHT) {
        if (delayRight != nullptr)
            delayRight->setMix(mix);
    }
}

void StereoDelay::setFeedback(StereoDelay::Channel channel, float fb) {
    if (channel == LEFT) {
        if (delayLeft != nullptr)
            delayLeft->setFeedback(fb);
    }
    else if (channel == RIGHT) {
        if (delayRight != nullptr)
            delayRight->setFeedback(fb);
    }
}

void StereoDelay::resetDelay() {
    if (delayLeft != nullptr)
        delayLeft->resetDelay();
     if (delayRight != nullptr)
         delayRight->resetDelay();
    
}
