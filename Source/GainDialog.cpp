#include "GainDialog.h"

GainDialog::GainDialog()
{
    setupComponents();
    setSize(400, 180);
}

GainDialog::~GainDialog()
{
}

void GainDialog::setupComponents()
{
    // Gain Label and Editor
    gainLabel.setText("Gain (dB):", juce::dontSendNotification);
    gainLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(gainLabel);
    
    gainEditor.setText("0.0");
    gainEditor.setInputRestrictions(0, "-0123456789.");
    gainEditor.onTextChange = [this]() { updateInfo(); };
    addAndMakeVisible(gainEditor);
    
    gainUnitLabel.setText("dB", juce::dontSendNotification);
    gainUnitLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(gainUnitLabel);
    
    // Info Label
    infoLabel.setText("Linear gain: 1.00x", juce::dontSendNotification);
    infoLabel.setJustificationType(juce::Justification::centred);
    infoLabel.setFont(juce::Font(12.0f));
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

void GainDialog::updateInfo()
{
    float gainDb = gainEditor.getText().getFloatValue();
    float linearGain = juce::Decibels::decibelsToGain(gainDb);
    
    juce::String infoText = "Linear gain: " + juce::String(linearGain, 2) + "x";
    
    if (gainDb > 0)
        infoText += " (Boost)";
    else if (gainDb < 0)
        infoText += " (Reduce)";
    
    infoLabel.setText(infoText, juce::dontSendNotification);
}

void GainDialog::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff222222));
    
    g.setColour(juce::Colours::white);
    g.setFont(16.0f);
    g.drawText("Apply Gain", 10, 10, getWidth() - 20, 30, juce::Justification::centred);
}

void GainDialog::resized()
{
    auto bounds = getLocalBounds().reduced(20);
    bounds.removeFromTop(40); // Space for title
    
    // Gain input
    auto gainRow = bounds.removeFromTop(30);
    gainLabel.setBounds(gainRow.removeFromLeft(100));
    gainEditor.setBounds(gainRow.removeFromLeft(120));
    gainRow.removeFromLeft(10);
    gainUnitLabel.setBounds(gainRow.removeFromLeft(40));
    
    bounds.removeFromTop(10); // Spacing
    
    // Info label
    auto infoRow = bounds.removeFromTop(25);
    infoLabel.setBounds(infoRow);
    
    bounds.removeFromTop(20); // Spacing
    
    // Buttons
    auto buttonRow = bounds.removeFromTop(30);
    buttonRow.removeFromLeft((buttonRow.getWidth() - 220) / 2); // Center buttons
    applyButton.setBounds(buttonRow.removeFromLeft(100));
    buttonRow.removeFromLeft(20);
    cancelButton.setBounds(buttonRow.removeFromLeft(100));
}
