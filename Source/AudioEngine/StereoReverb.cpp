/*
  ==============================================================================

    StereoReverb.cpp
    Created: 4 Dec 2016 4:15:58pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "StereoReverb.h"

StereoReverb::StereoReverb() {
    this->reverb = new Reverb();
}

StereoReverb::~StereoReverb() {
    reverb = nullptr;
}

void StereoReverb::processStereo(float *const left, float *const right, const int numSamples) {
    if (this->enabled) {
        reverb->processStereo(left, right, numSamples);
    }
}

void StereoReverb::setParameters (const juce::Reverb::Parameters& newParams) {
    reverb->setParameters(newParams);
}

const juce::Reverb::Parameters& StereoReverb::getParameters() {
    return reverb->getParameters();
}

void StereoReverb::reset() {
    reverb->reset();
}