#pragma once

#include <JuceHeader.h>

/**
 * FadeDialog - Dialog for Fade In/Out settings
 * 
 * This dialog allows the user to configure fade parameters:
 * - Fade In duration (in milliseconds or percentage)
 * - Fade Out duration (in milliseconds or percentage)
 * - Fade type (Linear or Exponential)
 */
class FadeDialog : public juce::Component
{
public:
    FadeDialog();
    ~FadeDialog() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Getters for fade parameters
    int getFadeInMs() const { return fadeInEditor.getText().getIntValue(); }
    int getFadeOutMs() const { return fadeOutEditor.getText().getIntValue(); }
    bool isExponential() const { return fadeTypeCombo.getSelectedId() == 2; }
    
    // Callback when "Apply" is clicked
    std::function<void()> onApply;
    std::function<void()> onCancel;

private:
    // UI Components
    juce::Label fadeInLabel;
    juce::TextEditor fadeInEditor;
    juce::Label fadeInUnitLabel;
    
    juce::Label fadeOutLabel;
    juce::TextEditor fadeOutEditor;
    juce::Label fadeOutUnitLabel;
    
    juce::Label fadeTypeLabel;
    juce::ComboBox fadeTypeCombo;
    
    juce::TextButton applyButton;
    juce::TextButton cancelButton;
    
    void setupComponents();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FadeDialog)
};
