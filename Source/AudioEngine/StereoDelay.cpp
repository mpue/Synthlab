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
            left[sample] = delayLeft->next(left[sample]);
            right[sample] = delayRight->next(right[sample]);
        }
    }
}

void StereoDelay::setDelay(StereoDelay::Channel channel, float time) {

	Logger::getCurrentLogger()->writeToLog("Setting delayTime for channel " + String(channel) + " to " + String(time) + "ms");

    if (channel == LEFT) {
        delayLeft->setDelay(time);
    }
    else if (channel == RIGHT) {
        delayRight->setDelay(time);
    }
}

void StereoDelay::setMix(StereoDelay::Channel channel, float mix) {
    if (channel == LEFT) {
        delayLeft->setMix(mix);
    }
    else if (channel == RIGHT) {
        delayRight->setMix(mix);
    }
}

void StereoDelay::setFeedback(StereoDelay::Channel channel, float fb) {
    if (channel == LEFT) {
        delayLeft->setFeedback(fb);
    }
    else if (channel == RIGHT) {
        delayRight->setFeedback(fb);
    }
}

void StereoDelay::resetDelay() {
    delayLeft->resetDelay();
    delayRight->resetDelay();
    
}
