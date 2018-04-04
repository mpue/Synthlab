/*
  ==============================================================================

    Modulation.h
    Created: 10 Dec 2016 9:55:40am
    Author:  Matthias Pueski

  ==============================================================================
*/

#ifndef MODULATION_H_INCLUDED
#define MODULATION_H_INCLUDED

#include "Modulator.h"
#include "ModTarget.h"
#include <vector>


using namespace std;

class Modulation {
    
    
public:
     Modulation(Modulator* modulator, ModTarget *target);
     Modulation();
     ~Modulation();
    
    vector<ModTarget*> getTargets();
    
    Modulator* getModulator();
    void addTarget(ModTarget* target);
    void setModulator(Modulator* modulator);
    void process();
    void setEnabled(bool enabled);
    bool isEnabled();
    
    
private:
    
    vector<ModTarget*> targets;
    Modulator* modulator = NULL;
    bool enabled = false;
    
};




#endif  // MODULATION_H_INCLUDED
