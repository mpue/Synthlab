#pragma once

#include <JuceHeader.h>

/**
 * BounceDialog - Dialog für Audio Export Einstellungen
 * 
 * Dieser Dialog ermöglicht es dem Benutzer, die wichtigsten Parameter
 * für das Audio Bouncing zu konfigurieren:
 * - Ausgabedatei auswählen
 * - Dauer in Sekunden festlegen
 * - Sample Rate wählen
 * - Bit Depth festlegen
 * - Optional: Start- und Endposition
 */
class BounceDialog : public juce::Component
{
public:
    BounceDialog();
    ~BounceDialog() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Getter für die Bounce-Parameter
    juce::File getOutputFile() const { return outputFile; }
    double getDuration() const { return durationEditor.getText().getDoubleValue(); }
    int getSampleRate() const { return sampleRateCombo.getSelectedId() == 1 ? 44100 : 48000; }
    int getBitDepth() const { return bitDepthCombo.getSelectedId() == 1 ? 16 : 24; }
    bool useMarkerRange() const { return useRangeButton.getToggleState(); }
    
    // Callback wenn "Export" geklickt wird
    std::function<void()> onExport;
    std::function<void()> onCancel;

private:
    // UI Komponenten
    juce::Label fileLabel;
    juce::TextButton browseButton;
    juce::Label filePathLabel;
    
    juce::Label durationLabel;
    juce::TextEditor durationEditor;
    
    juce::Label sampleRateLabel;
    juce::ComboBox sampleRateCombo;
    
    juce::Label bitDepthLabel;
    juce::ComboBox bitDepthCombo;
    
    juce::ToggleButton useRangeButton;
    
    juce::TextButton exportButton;
    juce::TextButton cancelButton;
    
    // Daten
    juce::File outputFile;
    
    void browseForFile();
    void setupComponents();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BounceDialog)
};
