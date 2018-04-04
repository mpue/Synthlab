/*
  ==============================================================================

    Note.h
    Created: 4 Jun 2016 6:34:13pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#ifndef NOTE_H_INCLUDED
#define NOTE_H_INCLUDED

class Note {
    
public:
    
    Note();
    ~Note();
    
    void setVelocity(int velocity);
    int getVelocity() const;
    void setMidiNote(int note);
    int getMidiNote() const;
    void setPlaying(bool playing);
    bool isPlaying() const;

    
private:
    int velocity;
    int midiNote;
    bool playing;

    
};



#endif  // NOTE_H_INCLUDED
