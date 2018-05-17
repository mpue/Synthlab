/*
  ==============================================================================

    FractionalDelayBuffer.h

  ==============================================================================
*/

#ifndef FRACTIONALDELAYBUFFER_H_INCLUDED
#define FRACTIONALDELAYBUFFER_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"
#include <iostream>

class FractionalDelayBuffer
{
public:
    FractionalDelayBuffer();
    ~FractionalDelayBuffer();
    
    void clear();
    void setBufferSize(int size);
    void addSample(float sample);
    float getSample(float sampleIndex);
    int getValidIndex(int index);
    float linearInterpolate(float* buffer, int bufferSize, int bufferPosition);
    
private:
    int index;
    int bufferSize;
    float* buffer;
};

#endif  // FRACTIONALDELAYBUFFER_H_INCLUDED
