#include "BounceDialog.h"

BounceDialog::BounceDialog()
{
    setupComponents();
    setSize(500, 400);
}

BounceDialog::~BounceDialog()
{
}

void BounceDialog::setupComponents()
{
    // Dateiauswahl
    fileLabel.setText("Output File:", juce::dontSendNotification);
    fileLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(fileLabel);
    
    browseButton.setButtonText("Browse...");
    browseButton.onClick = [this] { browseForFile(); };
    addAndMakeVisible(browseButton);
    
    filePathLabel.setText("No file selected", juce::dontSendNotification);
    filePathLabel.setJustificationType(juce::Justification::centredLeft);
    filePathLabel.setColour(juce::Label::backgroundColourId, juce::Colours::darkgrey.darker());
    filePathLabel.setColour(juce::Label::outlineColourId, juce::Colours::grey);
    addAndMakeVisible(filePathLabel);
    
    // Dauer
    durationLabel.setText("Duration (seconds):", juce::dontSendNotification);
    durationLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(durationLabel);
    
    durationEditor.setText("60.0");
    durationEditor.setInputRestrictions(10, "0123456789.");
    durationEditor.setJustification(juce::Justification::centred);
    addAndMakeVisible(durationEditor);
    
    // Sample Rate
    sampleRateLabel.setText("Sample Rate:", juce::dontSendNotification);
    sampleRateLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(sampleRateLabel);
    
    sampleRateCombo.addItem("44100 Hz", 1);
    sampleRateCombo.addItem("48000 Hz", 2);
    sampleRateCombo.setSelectedId(1);
    addAndMakeVisible(sampleRateCombo);
    
    // Bit Depth
    bitDepthLabel.setText("Bit Depth:", juce::dontSendNotification);
    bitDepthLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(bitDepthLabel);
    
    bitDepthCombo.addItem("16 Bit", 1);
    bitDepthCombo.addItem("24 Bit", 2);
    bitDepthCombo.setSelectedId(2);
    addAndMakeVisible(bitDepthCombo);
    
    // Marker Range Option
    useRangeButton.setButtonText("Use Marker Range (if set)");
    useRangeButton.setToggleState(false, juce::dontSendNotification);
    addAndMakeVisible(useRangeButton);
    
    // Buttons
    exportButton.setButtonText("Start Bounce");
    exportButton.onClick = [this] {
        if (onExport)
            onExport();
    };
    addAndMakeVisible(exportButton);
    
    cancelButton.setButtonText("Cancel");
    cancelButton.onClick = [this] {
        if (onCancel)
            onCancel();
    };
    addAndMakeVisible(cancelButton);
}

void BounceDialog::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff222222));
    
    g.setColour(juce::Colours::white);
    g.setFont(16.0f);
    g.drawText("Export Audio Settings", getLocalBounds().removeFromTop(40), 
               juce::Justification::centred, true);
}

void BounceDialog::resized()
{
    auto bounds = getLocalBounds().reduced(20);
    bounds.removeFromTop(50); // Für Titel
    
    const int rowHeight = 30;
    const int spacing = 10;
    const int labelWidth = 150;
    
    // Dateiauswahl
    auto fileRow = bounds.removeFromTop(rowHeight);
    fileLabel.setBounds(fileRow.removeFromLeft(labelWidth));
    browseButton.setBounds(fileRow.removeFromRight(100));
    
    bounds.removeFromTop(spacing);
    filePathLabel.setBounds(bounds.removeFromTop(rowHeight));
    
    bounds.removeFromTop(spacing);
    
    // Dauer
    auto durationRow = bounds.removeFromTop(rowHeight);
    durationLabel.setBounds(durationRow.removeFromLeft(labelWidth));
    durationEditor.setBounds(durationRow.removeFromLeft(100));
    
    bounds.removeFromTop(spacing);
    
    // Sample Rate
    auto sampleRateRow = bounds.removeFromTop(rowHeight);
    sampleRateLabel.setBounds(sampleRateRow.removeFromLeft(labelWidth));
    sampleRateCombo.setBounds(sampleRateRow.removeFromLeft(150));
    
    bounds.removeFromTop(spacing);
    
    // Bit Depth
    auto bitDepthRow = bounds.removeFromTop(rowHeight);
    bitDepthLabel.setBounds(bitDepthRow.removeFromLeft(labelWidth));
    bitDepthCombo.setBounds(bitDepthRow.removeFromLeft(150));
    
    bounds.removeFromTop(spacing);
    
    // Marker Range
    useRangeButton.setBounds(bounds.removeFromTop(rowHeight));
    
    bounds.removeFromTop(spacing * 2);
    
    // Buttons
    auto buttonRow = bounds.removeFromTop(40);
    cancelButton.setBounds(buttonRow.removeFromRight(100));
    buttonRow.removeFromRight(10);
    exportButton.setBounds(buttonRow.removeFromRight(100));
}

void BounceDialog::browseForFile()
{
    juce::FileChooser chooser("Select output file for bounce",
                              juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
                              "*.wav");
    
    if (chooser.browseForFileToSave(true))
    {
        outputFile = chooser.getResult();
        
        // Stelle sicher, dass die Datei die .wav Endung hat
        if (!outputFile.hasFileExtension(".wav"))
            outputFile = outputFile.withFileExtension(".wav");
        
        filePathLabel.setText(outputFile.getFullPathName(), juce::dontSendNotification);
    }
}
