/*
  ==============================================================================

    Voice.h
    Created: 4 Jun 2016 6:53:13pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#ifndef VOICE_H_INCLUDED
#define VOICE_H_INCLUDED

#include <vector>

#include "Oszillator.h"
#include "Sawtooth.h"
#include "Modulator.h"
#include "Note.h"
#include "ADSR.h"
#include "ModTarget.h"

#include "../JuceLibraryCode/JuceHeader.h"

using namespace std;

class Voice : public ModTarget {
    
public:
        
    Voice(float sampleRate);
    ~Voice();
    
    void addOszillator(Oszillator* o);
    vector<Oszillator*> getOszillators() const;
    Oszillator* getOscillator(int num);
    
    float process();
    
    void setNoteNumber(int number);
    int getNoteNumber() const;
    void setOctave(int number);
    int getOctave() const;
    
    void setOffset(int number);
    
    void setPitch(int number);
    int getPitch() const;
    void updateOscillator(int index);
    
    void setPlaying(bool playing);
    bool isPlaying() const;

    virtual void setModulator(Modulator* mod) override;
    virtual void setModAmount(float amount) override;
    
    ADSR* getAmpEnvelope();

    float getSampleRate();
    
    void setPitchBend(float bend);
    
    void setDuration(float duration);
    float getDuration();
    void setTime(float time);
    float getTime();
    
	void setVelocity(int velocity);
	int getVelocity() const;
	void setNoteAndVelocity(int note, int velocity);

private:
    vector<Oszillator*> oscillators;
    float sampleRate;
    int noteNumber;
    int pitch;
    int velocity;
    double midiNote[128];
    void calculateFrequencyTable();
    bool playing;
    ADSR* ampEnvelope;
    Modulator* modulator;
    float pitchBend;
    float duration;
    float time;
    int octave = 0;
    int offset = 0;
	float value = 0;
    float modAmount;

};



#endif  // VOICE_H_INCLUDED
