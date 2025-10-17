#include "SilenceDialog.h"

SilenceDialog::SilenceDialog()
{
    setupComponents();
    setSize(400, 200);
}

SilenceDialog::~SilenceDialog()
{
}

void SilenceDialog::setupComponents()
{
    // Mode Selection
    modeLabel.setText("Mode:", juce::dontSendNotification);
    modeLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(modeLabel);
    
    modeCombo.addItem("Insert at Beginning", 1);
    modeCombo.addItem("Insert at End", 2);
    modeCombo.addItem("Replace Region with Silence", 3);
    modeCombo.setSelectedId(3); // Default: Replace
    addAndMakeVisible(modeCombo);
    
    // Duration
    durationLabel.setText("Duration (ms):", juce::dontSendNotification);
    durationLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(durationLabel);
    
    durationEditor.setText("1000");
    durationEditor.setInputRestrictions(6, "0123456789");
    addAndMakeVisible(durationEditor);
    
    durationUnitLabel.setText("ms", juce::dontSendNotification);
    durationUnitLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(durationUnitLabel);
    
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

void SilenceDialog::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff222222));
    
    g.setColour(juce::Colours::white);
    g.setFont(16.0f);
    g.drawText("Insert Silence", 10, 10, getWidth() - 20, 30, juce::Justification::centred);
}

void SilenceDialog::resized()
{
    auto bounds = getLocalBounds().reduced(20);
    bounds.removeFromTop(40); // Space for title
    
    // Mode selection
    auto modeRow = bounds.removeFromTop(30);
    modeLabel.setBounds(modeRow.removeFromLeft(120));
    modeCombo.setBounds(modeRow.removeFromLeft(220));
    
    bounds.removeFromTop(10); // Spacing
    
    // Duration
    auto durationRow = bounds.removeFromTop(30);
    durationLabel.setBounds(durationRow.removeFromLeft(120));
    durationEditor.setBounds(durationRow.removeFromLeft(100));
    durationRow.removeFromLeft(10);
    durationUnitLabel.setBounds(durationRow.removeFromLeft(40));
    
    bounds.removeFromTop(20); // Spacing
    
    // Buttons
    auto buttonRow = bounds.removeFromTop(30);
    buttonRow.removeFromLeft((buttonRow.getWidth() - 220) / 2); // Center buttons
    applyButton.setBounds(buttonRow.removeFromLeft(100));
    buttonRow.removeFromLeft(20);
    cancelButton.setBounds(buttonRow.removeFromLeft(100));
}
