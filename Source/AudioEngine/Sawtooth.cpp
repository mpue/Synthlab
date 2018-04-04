/*
  ==============================================================================

    Sawtooth.cpp
    Created: 3 Jun 2016 9:02:07pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "Sawtooth.h"

#define _USE_MATH_DEFINES

#include <math.h>
#include <iostream>

using namespace std;

Sawtooth::Sawtooth(float sampleRate) : Oszillator(sampleRate) {
    this->volume = 1.0f;
    this->frequency = 440.0f;
    this->fine = 0.0f;
    this->p = 0.0f;      //current position
    this->dp = 1.0f;     //change in postion per sample
    this->leak = 0.995f; //leaky integrator
    this->pmax = 0.5f * sampleRate / (frequency + this->fine);
    this->dc = -0.498f / this->pmax;
    this->lastValue = 0;
	this->saw = 0;
}

float Sawtooth::getOutput() {
    return this->saw;
}

void Sawtooth::reset() {
    this->p = 0;
}


float Sawtooth::process() {
    
    p += dp;
    
    if(p < 0.0f)
    {
        p = -p;
        dp = -dp;
    }
    else if(p > pmax)
    {
        p = pmax + pmax - p;
        dp = -dp;
        
        if (this->slave != 0 && sync) {
            slave->reset();
        }
    }
    
    x = M_PI * p;
    
    if(x < 0.00001f)
        x=0.00001f; //don't divide by 0
    
    saw = leak * saw + dc + (float)sin(x)/(x);
    
    if (saw > 1.0f)
        saw = lastValue;
    else
        lastValue = saw;
        
    return saw * this->volume;
}

void Sawtooth::setFrequency(double frequency) {
    this->frequency = frequency;
    pmax = 0.5f * sampleRate / (frequency + this->fine);
    dc = -0.498f / pmax;
    
}

void Sawtooth::setFine(float fine) {
    this->fine = fine;
    setFrequency(frequency);
}

float Sawtooth::getFine() const {
    return this->fine;
}