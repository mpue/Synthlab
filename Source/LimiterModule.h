/*
  ==============================================================================

    LimiterModule.h
    Created: 28 Jun 2026
    Author:  mpue

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"

//==============================================================================
/*
    Mono peak limiter (brick-wall). The output is held below the ceiling using
    an instantaneous-attack / smoothed-release gain follower.

    Inputs:
        Audio In   - signal to limit
        Ceiling    - maximum output level in dB (default -0.3)
        Release    - release time in ms (default 50)

    Output:
        Audio Out  - limited signal
 */
class LimiterModule : public Module
{
public:
    LimiterModule(float sampleRate, int buffersize);
    ~LimiterModule();

    void paint(juce::Graphics& g) override;
    virtual void process() override;
    virtual void configurePins() override;

    virtual juce::String getCategory() override {
        return "FX";
    }

    virtual juce::String getDescription() override {
        return "Brick-wall peak limiter with adjustable ceiling and release";
    }

private:
    Pin* audioIn = nullptr;
    Pin* ceilingIn = nullptr;
    Pin* releaseIn = nullptr;
    Pin* audioOut = nullptr;

    // Current gain applied to the signal (linear, 1.0 = no reduction)
    float gain = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LimiterModule)
};
