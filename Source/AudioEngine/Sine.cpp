//
//  Sine.cpp
//  Trio
//
//  Created by Matthias Pueski on 20.11.16.
//
//

#include "Sine.h"

#define _USE_MATH_DEFINES

#include <math.h>
#include <iostream>

Sine::Sine(float sampleRate, int buffersize) : Oszillator(sampleRate) {
    this->volume = 1.0f;
    this->frequency = 440.0f;
    this->fine = 0.0f;
    this->step = (frequency * 2 * M_PI) / sampleRate;
    this->phase = 0;
	this->value = 0;
}

float Sine::getOutput() {
    return this->value;
}

void Sine::reset() {
    this->phase = 0;
}

float Sine::process() {
    
    phase += step;

    this->value = (float)(sin(phase) * this->volume);
    return this->value;
}

void Sine::setFrequency(double frequency) {
    this->frequency = frequency;
    this->step = ((frequency + (double)this->fine) * 2 * M_PI / sampleRate);
}

void Sine::setFine(float fine) {
    this->fine = fine;
    this->step = ((frequency + (double)this->fine) * 2 * M_PI / sampleRate);
}

float Sine::getFine() const {
    return this->fine;
}
