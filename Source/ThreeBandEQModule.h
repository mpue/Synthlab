/*
  ==============================================================================

    ThreeBandEQModule.h
    Created: [Current Date]
    Author:  [Author Name]

    3-Band Equalizer Module with Low, Mid, and High frequency bands
    Each band has adjustable frequency, Q-factor, and gain

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"

//==============================================================================
/*
    3-Band Equalizer Module
    
    Features:
    - Low, Mid, High frequency bands
    - Individual frequency control for each band
    - Q-factor control for each band  
    - Gain control for each band
    - Single audio input and output
*/
class ThreeBandEQModule : public Module
{
public:
    ThreeBandEQModule(double sampleRate, int bufferSize);
    ~ThreeBandEQModule();

    void paint (juce::Graphics& g) override;
    void process() override;
    virtual void configurePins() override;
    
    virtual juce::String getCategory() override {
        return "Filter";
    }
    
    virtual juce::String getDescription() override {
        return "3-Band Equalizer with Low, Mid, and High frequency bands. Each band has adjustable frequency, Q-factor, and gain controls.";
    }

private:
    // IIR filters for each band
    std::unique_ptr<juce::IIRFilter> lowShelfFilter;
    std::unique_ptr<juce::IIRFilter> midPeakFilter;
    std::unique_ptr<juce::IIRFilter> highShelfFilter;
    
    // Current parameter values
    struct BandParams {
        float frequency = 1000.0f;
        float q = 0.7f;
        float gain = 1.0f;
        bool needsUpdate = true;
    };
    
    BandParams lowBand = {100.0f, 0.7f, 1.0f, true};    // Low shelf at 100Hz
    BandParams midBand = {1000.0f, 0.7f, 1.0f, true};   // Mid peak at 1kHz  
    BandParams highBand = {10000.0f, 0.7f, 1.0f, true}; // High shelf at 10kHz
    
    // Working buffers for multi-stage processing
    juce::AudioBuffer<float> tempBuffer1;
    juce::AudioBuffer<float> tempBuffer2;
    
    // Update filter coefficients when parameters change
    void updateLowBandFilter();
    void updateMidBandFilter();
    void updateHighBandFilter();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ThreeBandEQModule)
};