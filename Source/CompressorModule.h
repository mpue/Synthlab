/*
  ==============================================================================

    CompressorModule.h
    Created: 28 Jun 2026
    Author:  mpue

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"

//==============================================================================
/*
    Mono dynamics compressor with feed-forward peak detection.

    Inputs:
        Audio In   - signal to compress
        Threshold  - level in dB above which compression starts (default -18)
        Ratio      - compression ratio, e.g. 4 means 4:1 (default 4)
        Attack     - attack time in ms (default 10)
        Release    - release time in ms (default 100)
        Makeup     - makeup gain in dB (default 0)

    Output:
        Audio Out  - compressed signal
 */
class CompressorModule : public Module
{
public:
    CompressorModule(float sampleRate, int buffersize);
    ~CompressorModule();

    void paint(juce::Graphics& g) override;
    virtual void process() override;
    virtual void configurePins() override;

    virtual juce::String getCategory() override {
        return "FX";
    }

    virtual juce::String getDescription() override {
        return "Dynamics compressor with threshold, ratio, attack, release and makeup gain";
    }

private:
    Pin* audioIn = nullptr;
    Pin* thresholdIn = nullptr;
    Pin* ratioIn = nullptr;
    Pin* attackIn = nullptr;
    Pin* releaseIn = nullptr;
    Pin* makeupIn = nullptr;
    Pin* audioOut = nullptr;

    // Envelope follower state (linear amplitude)
    float envelope = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompressorModule)
};
