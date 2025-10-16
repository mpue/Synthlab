#include "FadeDialog.h"

FadeDialog::FadeDialog()
{
    setupComponents();
    setSize(400, 220);
}

FadeDialog::~FadeDialog()
{
}

void FadeDialog::setupComponents()
{
    // Fade In Label and Editor
    fadeInLabel.setText("Fade In (ms):", juce::dontSendNotification);
    fadeInLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(fadeInLabel);
    
    fadeInEditor.setText("100");
    fadeInEditor.setInputRestrictions(6, "0123456789");
    addAndMakeVisible(fadeInEditor);
    
    fadeInUnitLabel.setText("ms", juce::dontSendNotification);
    fadeInUnitLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(fadeInUnitLabel);
    
    // Fade Out Label and Editor
    fadeOutLabel.setText("Fade Out (ms):", juce::dontSendNotification);
    fadeOutLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(fadeOutLabel);
    
    fadeOutEditor.setText("100");
    fadeOutEditor.setInputRestrictions(6, "0123456789");
    addAndMakeVisible(fadeOutEditor);
    
    fadeOutUnitLabel.setText("ms", juce::dontSendNotification);
    fadeOutUnitLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(fadeOutUnitLabel);
    
    // Fade Type
    fadeTypeLabel.setText("Fade Type:", juce::dontSendNotification);
    fadeTypeLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(fadeTypeLabel);
    
    fadeTypeCombo.addItem("Linear", 1);
    fadeTypeCombo.addItem("Exponential (Smooth)", 2);
    fadeTypeCombo.setSelectedId(1);
    addAndMakeVisible(fadeTypeCombo);
    
    // Buttons
    applyButton.setButtonText("Apply");
    applyButton.onClick = [this]() {
        if (onApply)
            onApply();
    };
    addAndMakeVisible(applyButton);
    
    cancelButton.setButtonText("Cancel");
    cancelButton.onClick = [this]() {
        if (onCancel)
            onCancel();
    };
    addAndMakeVisible(cancelButton);
}

void FadeDialog::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff222222));
    
    g.setColour(juce::Colours::white);
    g.setFont(16.0f);
    g.drawText("Fade In/Out Settings", 10, 10, getWidth() - 20, 30, juce::Justification::centred);
}

void FadeDialog::resized()
{
    auto bounds = getLocalBounds().reduced(20);
    bounds.removeFromTop(40); // Space for title
    
    // Fade In
    auto fadeInRow = bounds.removeFromTop(30);
    fadeInLabel.setBounds(fadeInRow.removeFromLeft(120));
    fadeInEditor.setBounds(fadeInRow.removeFromLeft(100));
    fadeInRow.removeFromLeft(10);
    fadeInUnitLabel.setBounds(fadeInRow.removeFromLeft(40));
    
    bounds.removeFromTop(10); // Spacing
    
    // Fade Out
    auto fadeOutRow = bounds.removeFromTop(30);
    fadeOutLabel.setBounds(fadeOutRow.removeFromLeft(120));
    fadeOutEditor.setBounds(fadeOutRow.removeFromLeft(100));
    fadeOutRow.removeFromLeft(10);
    fadeOutUnitLabel.setBounds(fadeOutRow.removeFromLeft(40));
    
    bounds.removeFromTop(10); // Spacing
    
    // Fade Type
    auto fadeTypeRow = bounds.removeFromTop(30);
    fadeTypeLabel.setBounds(fadeTypeRow.removeFromLeft(120));
    fadeTypeCombo.setBounds(fadeTypeRow.removeFromLeft(200));
    
    bounds.removeFromTop(20); // Spacing
    
    // Buttons
    auto buttonRow = bounds.removeFromTop(30);
    buttonRow.removeFromLeft((buttonRow.getWidth() - 220) / 2); // Center buttons
    applyButton.setBounds(buttonRow.removeFromLeft(100));
    buttonRow.removeFromLeft(20);
    cancelButton.setBounds(buttonRow.removeFromLeft(100));
}
