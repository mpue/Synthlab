#pragma once

#include <JuceHeader.h>

/**
 * TrimDialog - Dialog for trimming silence from audio regions
 * 
 * Allows the user to:
 * - Trim silence from beginning
 * - Trim silence from end
 * - Trim silence from both ends
 * - Set threshold level
 */
class TrimDialog : public juce::Component
{
public:
    TrimDialog();
    ~TrimDialog() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    enum class TrimMode
    {
        Beginning,
        End,
        Both
    };

    // Getters
    TrimMode getMode() const { return static_cast<TrimMode>(modeCombo.getSelectedId() - 1); }
    float getThreshold() const { return thresholdEditor.getText().getFloatValue(); }
    
    // Callbacks
    std::function<void()> onApply;
    std::function<void()> onCancel;

private:
    // UI Components
    juce::Label modeLabel;
    juce::ComboBox modeCombo;
    
    juce::Label thresholdLabel;
    juce::TextEditor thresholdEditor;
    juce::Label thresholdUnitLabel;
    
    juce::Label infoLabel;
    
    juce::TextButton applyButton;
    juce::TextButton cancelButton;
    
    void setupComponents();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrimDialog)
};
