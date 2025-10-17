#pragma once

#include <JuceHeader.h>

/**
 * GainDialog - Dialog for applying gain to audio regions
 * 
 * Allows the user to:
 * - Specify gain in dB or as a linear multiplier
 * - Preview the resulting level
 */
class GainDialog : public juce::Component
{
public:
    GainDialog();
    ~GainDialog() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Getters
    float getGainDb() const { return gainEditor.getText().getFloatValue(); }
    float getLinearGain() const { return juce::Decibels::decibelsToGain(getGainDb()); }
    
    // Callbacks
    std::function<void()> onApply;
    std::function<void()> onCancel;

private:
    // UI Components
    juce::Label gainLabel;
    juce::TextEditor gainEditor;
    juce::Label gainUnitLabel;
    
    juce::Label infoLabel;
    
    juce::TextButton applyButton;
    juce::TextButton cancelButton;
    
    void setupComponents();
    void updateInfo();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GainDialog)
};
