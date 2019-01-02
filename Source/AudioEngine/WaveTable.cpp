/*
  ==============================================================================

    WaveTable.cpp
    Created: 25 Dec 2018 3:14:02pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "WaveTable.h"

//
//  WaveTable.cpp
//  Trio
//
//  Created by Matthias Pueski on 23.11.16.
//
//

#include "WaveTable.h"

#define _USE_MATH_DEFINES

#include <math.h>
#include <iostream>

#include "../JuceLibraryCode/JuceHeader.h"

WaveTable::WaveTable(float sampleRate, int buffersize) : Oszillator(sampleRate) {
    this->volume = 1.0f;
    this->frequency = 440.0f;
    this->fine = 0.0f;
    this->step = (frequency * 2 * M_PI) / sampleRate;
    this->phase = 0;
}

void WaveTable::reset() {
    this->phase = 0;
}

float WaveTable::getOutput() {
    return this->value;
}

float WaveTable::process() {
    
    phase += step;
    

    
    return this->value * volume;
}

void WaveTable::setFrequency(double frequency) {
    this->frequency = frequency;
    this->step = ((frequency + (double)this->fine) * 2 * M_PI / sampleRate);
}

void WaveTable::setFine(float fine) {
    this->fine = fine;
    this->step = ((frequency + (double)this->fine) * 2 * M_PI / sampleRate);
}

float WaveTable::getFine() const {
    return this->fine;
}
