#include "TrimDialog.h"

TrimDialog::TrimDialog()
{
    setupComponents();
    setSize(400, 220);
}

TrimDialog::~TrimDialog()
{
}

void TrimDialog::setupComponents()
{
    // Mode Selection
    modeLabel.setText("Trim from:", juce::dontSendNotification);
    modeLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(modeLabel);
    
    modeCombo.addItem("Beginning", 1);
    modeCombo.addItem("End", 2);
    modeCombo.addItem("Both Ends", 3);
    modeCombo.setSelectedId(3); // Default: Both
    addAndMakeVisible(modeCombo);
    
    // Threshold
    thresholdLabel.setText("Threshold:", juce::dontSendNotification);
    thresholdLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(thresholdLabel);
    
    thresholdEditor.setText("0.001");
    thresholdEditor.setInputRestrictions(0, "0123456789.");
    addAndMakeVisible(thresholdEditor);
    
    thresholdUnitLabel.setText("(amplitude)", juce::dontSendNotification);
    thresholdUnitLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(thresholdUnitLabel);
    
    // Info label
    infoLabel.setText("Samples below this threshold will be considered silent", juce::dontSendNotification);
    infoLabel.setJustificationType(juce::Justification::centred);
    infoLabel.setFont(juce::Font(11.0f));
    addAndMakeVisible(infoLabel);
    
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

void TrimDialog::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff222222));
    
    g.setColour(juce::Colours::white);
    g.setFont(16.0f);
    g.drawText("Trim Silence", 10, 10, getWidth() - 20, 30, juce::Justification::centred);
}

void TrimDialog::resized()
{
    auto bounds = getLocalBounds().reduced(20);
    bounds.removeFromTop(40); // Space for title
    
    // Mode selection
    auto modeRow = bounds.removeFromTop(30);
    modeLabel.setBounds(modeRow.removeFromLeft(100));
    modeCombo.setBounds(modeRow.removeFromLeft(240));
    
    bounds.removeFromTop(10); // Spacing
    
    // Threshold
    auto thresholdRow = bounds.removeFromTop(30);
    thresholdLabel.setBounds(thresholdRow.removeFromLeft(100));
    thresholdEditor.setBounds(thresholdRow.removeFromLeft(100));
    thresholdRow.removeFromLeft(10);
    thresholdUnitLabel.setBounds(thresholdRow.removeFromLeft(120));
    
    bounds.removeFromTop(5); // Spacing
    
    // Info label
    auto infoRow = bounds.removeFromTop(25);
    infoLabel.setBounds(infoRow);
    
    bounds.removeFromTop(15); // Spacing
    
    // Buttons
    auto buttonRow = bounds.removeFromTop(30);
    buttonRow.removeFromLeft((buttonRow.getWidth() - 220) / 2); // Center buttons
    applyButton.setBounds(buttonRow.removeFromLeft(100));
    buttonRow.removeFromLeft(20);
    cancelButton.setBounds(buttonRow.removeFromLeft(100));
}
