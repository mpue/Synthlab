/*
  ==============================================================================

    Random.cpp
    Created: 13 Apr 2018 5:21:04pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "Random.h"



Synthlab::Random::Random(float sampleRate, int bufferSize) : Oszillator(sampleRate) {
    this->buffersize = bufferSize;
    currentTime = Time::getMillisecondCounterHiRes();
}

float Synthlab::Random::process() {
    
    float period = (1.0f/frequency) * 1000;
    
    if (Time::getMillisecondCounterHiRes() - currentTime >= period) {
        currentTime = Time::getMillisecondCounterHiRes();
        
        lastValue = random.nextFloat();
    }
    
    return lastValue * volume;
}

float Synthlab::Random::getOutput() {
    return 0;
}

void Synthlab::Random::setFrequency(double frequency){
    this->frequency = frequency;
}

void Synthlab::Random::reset() {
    currentTime = 0;
    lastValue = 0;
}



