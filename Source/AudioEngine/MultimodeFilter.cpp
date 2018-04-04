//
//  MultimodePassFilter.cpp
//  Trio
//
//  Created by Matthias Pueski on 16.11.16.
//
//


#include "MultimodeFilter.h"
#include "ADSR.h"

MultimodeFilter::MultimodeFilter() {

    this->lowPassLeft = new LowPassFilter();
    this->highPassLeft = new HighPassFilter();

    this->lowPassRight = new LowPassFilter();
    this->highPassRight = new HighPassFilter();
    
    this->mode = LOWPASS;
}

MultimodeFilter::~MultimodeFilter() {
    this->lowPassLeft = nullptr;
    this->lowPassRight = nullptr;
    this->highPassLeft = nullptr;
    this->highPassRight = nullptr;
}


void MultimodeFilter::setMode(Mode mode) {
    this->mode = mode;
}

void MultimodeFilter::coefficients(float frequency, float resonance) {
	if (frequency == 0) {
		frequency = 0.1;
	}
    this->lowPassLeft->coefficients( frequency, resonance);
    this->lowPassRight->coefficients( frequency, resonance);
    this->highPassLeft->coefficients( frequency, resonance);
    this->highPassRight->coefficients( frequency, resonance);
}

void MultimodeFilter::processStereo(float *const left, float *const right, const int numSamples) {
    if (this->enabled) {
        if (this->mode == Mode::LOWPASS) {
            this->lowPassLeft->process(left, 0, numSamples);
            this->lowPassRight->process(right, 0, numSamples);
        }
        else {
            this->highPassLeft->process(left, 0, numSamples);
            this->highPassRight->process(right, 0, numSamples);
        }
   
    }
}

void MultimodeFilter::setModulator(Modulator* mod) {
    this->lowPassLeft->setModulator(mod);
    this->lowPassRight->setModulator(mod);
    this->highPassLeft->setModulator(mod);
    this->highPassRight->setModulator(mod);
}

void MultimodeFilter::setModAmount(float amount) {
    this->lowPassLeft->setModAmount(amount);
    this->lowPassRight->setModAmount(amount);
    this->highPassLeft->setModAmount(amount);
    this->highPassRight->setModAmount(amount);
}
