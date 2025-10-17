#pragma once

#include <JuceHeader.h>

/**
 * SilenceDialog - Dialog for inserting silence into audio regions
 * 
 * Allows the user to:
 * - Insert silence at beginning or end
 * - Replace a section with silence
 * - Specify duration in milliseconds
 */
class SilenceDialog : public juce::Component
{
public:
    SilenceDialog();
    ~SilenceDialog() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    enum class SilenceMode
    {
        InsertBeginning,
        InsertEnd,
        Replace
    };

    // Getters
    int getDurationMs() const { return durationEditor.getText().getIntValue(); }
    SilenceMode getMode() const { return static_cast<SilenceMode>(modeCombo.getSelectedId() - 1); }
    
    // Callbacks
    std::function<void()> onApply;
    std::function<void()> onCancel;

private:
    // UI Components
    juce::Label modeLabel;
    juce::ComboBox modeCombo;
    
    juce::Label durationLabel;
    juce::TextEditor durationEditor;
    juce::Label durationUnitLabel;
    
    juce::TextButton applyButton;
    juce::TextButton cancelButton;
    
    void setupComponents();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SilenceDialog)
};
