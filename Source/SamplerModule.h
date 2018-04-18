/*
  ==============================================================================

    SamplerModule.h
    Created: 17 Apr 2018 3:34:36pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#pragma once


#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"

#include "AudioEngine/Sampler.h"

//==============================================================================
/*
 */
class SamplerModule : public Module, public Timer
{
public:
    SamplerModule(double sampleRate, int buffersize, AudioFormatManager* manager);
    ~SamplerModule();
    
    void paint (Graphics& g) override;
    
    void setAmplitude(float amplitude);
    void process() override;
    
    virtual void configurePins() override;
    virtual String getCategory() override {
        return "Sound sources";
    }
    
    virtual void timerCallback() override;
    virtual void eventReceived(Event *e) override;
    void loadSample(InputStream* is);
    void setSamplePath(String sample);
    String getSamplePath();
    
private:
    float samplePosX = 20;
    AudioFormatManager* manager;
    AudioThumbnailCache* cache = nullptr;
    AudioThumbnail* thumbnail = nullptr;
    Sampler* sampler = nullptr;
    int currentSample = 0;
    int currentEnvelope = -1;
    bool gate = false;
    String samplePath;
    
    float* bufferLeft;
    float* bufferRight;
    
    CatmullRomInterpolator* interpolatorLeft;
    CatmullRomInterpolator* interpolatorRight;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SamplerModule)
};
