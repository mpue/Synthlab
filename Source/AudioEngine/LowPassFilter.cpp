//
//  LowPassFilter.cpp
//  Trio
//
//  Created by Matthias Pueski on 16.11.16.
//
//


#include "LowPassFilter.h"
#include "ADSR.h"

LowPassFilter::LowPassFilter() {
    this->filter1 = new IIRFilter();
    this->filter2 = new IIRFilter();
    this->modulator = 0;
	this->modAmount = 0;
}

LowPassFilter::~LowPassFilter() {
    this->filter1 = nullptr;
    this->filter2 = nullptr;
}


void LowPassFilter::coefficients(float frequency, float resonance) {

    this->frequency = frequency;
    this->resonance = resonance;
    
    IIRCoefficients ic1  = IIRCoefficients::makeLowPass (44100, frequency, resonance);
    filter1->setCoefficients(ic1);
    filter2->setCoefficients(ic1);
}

void LowPassFilter::process(float *in, float *out, int numSamples) {
    
    float f = frequency;
    
    if (this->modulator != 0) {
    
        
        if (ADSR* env = dynamic_cast<ADSR*>(this->modulator)) {
        
            if(env->getState() != ADSR::env_idle) {
                f =  this->frequency + (modulator->getOutput() * this->modAmount * (22000 - this->frequency));
            }
            else {
                env->reset();
            }
            
        }
        else {
            // f =  this->frequency * modulator->getOutput() * this->modAmount;
            f =  this->frequency + (modulator->getOutput() * this->modAmount * 1000);
            // f = this->frequency * modulator->getOutput();
            cout << f << endl;
        }

        if (f <= 0) {
            f = 0.1;
        }
        if (f > 22000) {
            f = 22000;
        }

        IIRCoefficients ic1  = IIRCoefficients::makeLowPass (44100, f, this->resonance);

        filter1->setCoefficients(ic1);
        filter2->setCoefficients(ic1);
    }

    this->filter1->processSamples(in,numSamples);
    // in -= numSamples;
    // this->filter2->processSamples(in,numSamples);
}

void LowPassFilter::setModulator(Modulator* mod) {
    this->modulator = mod;
}

void LowPassFilter::setModAmount(float amount) {
    this->modAmount = amount;
}