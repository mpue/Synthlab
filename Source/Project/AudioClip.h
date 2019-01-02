/*
  ==============================================================================

    Sample.h
    Created: 31 Dec 2016 10:00:19am
    Author:  Matthias Pueski

  ==============================================================================
*/

#ifndef SAMPLE_H_INCLUDED
#define SAMPLE_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"

/**
 *  This class contains the information about the audio clips used by the project and
 *  the containing tracks. It is just used to persist the information about the clips
 *  being used by the project to disk.
 */

class AudioClip {
    
public:
    
    AudioClip(String refId, String name, long length, long offset);
    AudioClip();
    ~AudioClip();
    
    long getLength();
    void setLength(long length);
    
    String getName();
    void setName(String name);
    
    String getRefId();
    void setRefId(String path);
    
    long getOffset();
    void setOffset(long offset);
    
    ValueTree getConfig();
    
private:
    // the length of this clip in samples
    long length;
    
    // the name of the clip
    String name;
    
    // the origin of this clip, which is id of the project audio file
    String refId;
    
    // the offset of this clip relative to the track start in samples
    long offset;
    
    
};




#endif  // SAMPLE_H_INCLUDED
