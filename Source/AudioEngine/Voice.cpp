/*
  ==============================================================================

    Voice.cpp
    Created: 4 Jun 2016 6:53:08pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "Voice.h"
#include "Math.h"

Voice::Voice(float sampleRate) {

    this->sampleRate = sampleRate;
    this->calculateFrequencyTable();
    this->playing = false;
    this->ampEnvelope = new ADSR();
    this->modulator = 0;
    this->pitchBend = 1;
    
    ampEnvelope->setAttackRate(0 * sampleRate);  // 1 second
    ampEnvelope->setDecayRate(0 * sampleRate);
    ampEnvelope->setReleaseRate(0 * sampleRate);
    ampEnvelope->setSustainLevel(.8);
}

Voice::~Voice() {
	if (ampEnvelope != NULL)
		delete ampEnvelope;
    
    for(std::vector<Oszillator*>::iterator it = oscillators.begin(); it != oscillators.end(); ++it) {
        delete *it;
    }
    oscillators.clear();
}

void Voice::setNoteAndVelocity(int note, int velocity) {

	this->noteNumber = note;
	this->velocity = velocity;

	oscillators.at(0)->setFrequency((midiNote[noteNumber + oscillators.at(0)->getPitch()]) * pitchBend);
	oscillators.at(1)->setFrequency((midiNote[noteNumber + oscillators.at(1)->getPitch()]) * pitchBend);
	oscillators.at(2)->setFrequency((midiNote[noteNumber + oscillators.at(2)->getPitch()]) * pitchBend);

	/*
    for(std::vector<Oszillator*>::iterator it = oscillators.begin(); it != oscillators.end(); ++it) {
        Oszillator* o = *it;
        
        if (note != NULL)
            o->setFrequency((midiNote[noteNumber + o->getPitch()]) * pitchBend);
    }
	*/
}



void Voice::setPitchBend(float bend) {
    this->pitchBend = bend;
    for(std::vector<Oszillator*>::iterator it = oscillators.begin(); it != oscillators.end(); ++it) {
        Oszillator* o = *it;
        o->setFrequency((midiNote[noteNumber + o->getPitch()]) * pitchBend);
    }
}

void Voice::addOszillator(Oszillator* o) {
    this->oscillators.push_back(o);
}

vector<Oszillator*> Voice::getOszillators() const {
    return this->oscillators;
}

Oszillator* Voice::getOscillator(int num) {
    return oscillators.at(num);
}

float Voice::process() {
    
    value = 0;
    
    if(ampEnvelope->getState() != ADSR::env_idle) {
        
        float amplitude = (1.0f / (float) 127) * this->velocity;

        /*
        for(std::vector<Oszillator*>::iterator it = oscillators.begin(); it != oscillators.end(); ++it) {
            Oszillator* o = *it;
            value += o->process();
        }
		*/
        
        
        for (int i = 0; i < oscillators.size(); i++) {
            
            
			value += oscillators.at(i)->process();
		}


        value = (value / oscillators.size()) * amplitude * ampEnvelope->process();
        
    }
    else {
        ampEnvelope->reset();
    }

    if (modulator != 0) {
        
    }

    return value;
    
}

void Voice::setNoteNumber(int number) {
    this->noteNumber = number;
}

int Voice::getNoteNumber() const {
    return this->noteNumber;
}

void Voice::setPitch(int number) {
    this->pitch = number;
}

int Voice::getPitch() const {
    return this->pitch;
}

void Voice::setOctave(int number) {
    this->octave = number;
    for(std::vector<Oszillator*>::iterator it = oscillators.begin(); it != oscillators.end(); ++it) {
        Oszillator* o = *it;
        o->setFrequency((midiNote[noteNumber + o->getPitch() + this->offset + this->octave * 12]) * pitchBend);
    }
}

void Voice::setOffset(int number) {
    this->offset = number;
    for(std::vector<Oszillator*>::iterator it = oscillators.begin(); it != oscillators.end(); ++it) {
        Oszillator* o = *it;
		o->setFrequency((midiNote[noteNumber + o->getPitch() + this->offset + this->octave * 12]) * pitchBend);
    }
}

int Voice::getOctave() const {
    return this->octave;
}

void Voice::updateOscillator(int index) {
    oscillators.at(index)->setFrequency(midiNote[this->noteNumber + oscillators.at(index)->getPitch()]);
}

void Voice::calculateFrequencyTable() {
    int a = 440; // a is 440 hz...
    for (int x = 0; x < 127; ++x)
    {
        midiNote[x] = a * pow(2.0,(x-69.0)/12.0);
    }
}

void Voice::setPlaying(bool playing) {
    this->playing = playing;
}

bool Voice::isPlaying() const {
    return this->playing;
}


float Voice::getSampleRate() {
    return this->sampleRate;
}

ADSR* Voice::getAmpEnvelope() {
    return ampEnvelope;
}

void Voice::setModAmount(float amount ) {
    this->modAmount = amount;
}

void Voice::setModulator(Modulator* modulator) {
    this->modulator = modulator;
}

float Voice::getTime() {
    return this->time;
}

void Voice::setTime(float time) {
    this->time = time;
}

float Voice::getDuration() {
    return this->duration;
}

void Voice::setDuration(float duration) {
    this->duration = duration;
}

void Voice::setVelocity(int velocity) {
	this->velocity = velocity;
}

int Voice::getVelocity() const {
	return this->velocity;
}
