/*
  ==============================================================================

    Cell.h
    Created: 29 Mar 2018 10:25:06am
    Author:  Matthias Pueski

  ==============================================================================
*/

#pragma once

class Cell {
    
public:
    
    Cell();
    Cell(int note, int velocity, int length, bool enabled);
    
    void setNote(int value);
    int getNote();
    
    void setVelocity(int value);
    int getVelocity();
    
    void setLength(int value);
    int getLength();
    
    void setEnabled(bool value);
    bool isEnabled();
    
    
private:
    int note;
    int velocity;
    bool enabled;
    int length;
    
};
