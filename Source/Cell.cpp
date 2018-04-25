/*
  ==============================================================================

    Cell.cpp
    Created: 29 Mar 2018 10:25:06am
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "Cell.h"

Cell::Cell() {
    
}

Cell::Cell(int note, int velocity, int length, bool enabled) {
    this->note = note;
    this->velocity = velocity;
    this->length = length;
    this->enabled = enabled;
}

int Cell::getNote() {
    return note;
}

void Cell::setNote(int value) {
    this->note = value;
}

int Cell::getLength() {
    return length;
}

void Cell::setLength(int value) {
    this->length = value;
}

int Cell::getVelocity() {
    return velocity;
}

void Cell::setVelocity(int value) {
    this->velocity = value;
}

bool Cell::isEnabled() {
    return enabled;
}

void Cell::setEnabled(bool value) {
    this->enabled = value;
}
