/*
  ==============================================================================

    Note.cpp
    Created: 4 Jun 2016 6:34:04pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "Note.h"

Note::Note() {
    this->velocity = 0;
    this->midiNote = 0;
    this->playing = false;
}

Note::~Note() {
    
}

void Note::setVelocity(int velocity) {
    this->velocity = velocity;
}

int Note::getVelocity() const {
    return this->velocity;
}

void Note::setMidiNote(int note) {
    this->midiNote = note;
}

int Note::getMidiNote() const {
    return this->midiNote;
}

void Note::setPlaying(bool playing) {
    this->playing = playing;
}

bool Note::isPlaying() const {
    return this->playing;
}