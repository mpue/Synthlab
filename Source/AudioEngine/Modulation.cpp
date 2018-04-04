/*
  ==============================================================================

    Modulation.cpp
    Created: 10 Dec 2016 9:55:40am
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "Modulation.h"

Modulation::Modulation() {
}

Modulation::~Modulation() {

    /*
    for (int i = 0; i < targets.size();i++) {
        targets.at(i)->setModulator(NULL);
    }
    
    
    for(std::vector<ModTarget*>::iterator it = targets.begin(); it != targets.end(); ++it) {
        delete *it;
    }
    */
    
    this->targets.clear();
}

Modulation::Modulation(Modulator* modulator, ModTarget* target) {
    this->modulator = modulator;
    target->setModulator(modulator);
    this->targets.push_back(target);
}

void Modulation::process() {
    if (this->modulator != NULL) {
        this->modulator->process();
    }
}

Modulator* Modulation::getModulator() {
    return this->modulator;
}

void Modulation::setModulator(Modulator *modulator) {
    this->modulator = modulator;
}

void Modulation::addTarget(ModTarget* target) {
    target->setModulator(this->modulator);
    this->targets.push_back(target);
}

vector<ModTarget*> Modulation::getTargets() {
    return targets;
}

void Modulation::setEnabled(bool enabled) {
    this->enabled = enabled;
}

bool Modulation::isEnabled() {
    return this->enabled;
}