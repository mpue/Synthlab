/*
 ==============================================================================
 
 MidiGate.h
 Created: 4 Apr 2018 3:48:23pm
 Author:  Matthias Pueski
 
 ==============================================================================
 */

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"

class StereoReverb;

//==============================================================================
/*
 */
class ReverbModule : public Module
{
public:
    ReverbModule(double sampleRate, int buffersize);
    ~ReverbModule();
    
    void paint (juce::Graphics& g) override;
    
    void process() override;
    
    void setDamping(float damping);
    float getDamping();
 
    void setRoomSize(float size);
    float getRoomSize();
    
    void setWetLevel(float level);
    float getWetLevel();
    
    void setDryLevel(float level);
    float getDryLevel();
    
    void setFeedbackLevel(float level);
    float getFeedbackLevel();
    
    virtual void configurePins() override;
    virtual juce::String getCategory() override {
        return "FX";
    }
    
private:
    
    juce::Image* image;
    
    int currentSample = 0;
    
    float* bufferLeft;
    float* bufferRight;
    
    Reverb* reverb = nullptr;
    Reverb::Parameters params;
    
    
    float roomSize;     /**< Room size, 0 to 1.0, where 1.0 is big, 0 is small. */
    float damping;      /**< Damping, 0 to 1.0, where 0 is not damped, 1.0 is fully damped. */
    float wetLevel;     /**< Wet level, 0 to 1.0 */
    float dryLevel;     /**< Dry level, 0 to 1.0 */
    float width;        /**< Reverb width, 0 to 1.0, where 1.0 is very wide. */
    float freezeMode;   /**< Freeze mode - values < 0.5 are "normal" mode, values > 0.5
                         put the reverb into a continuous feedback loop. */
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReverbModule)
};


