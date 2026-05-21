/*
  ==============================================================================

    RadioGroupModule.h
    Created: 2026
    Author:  Matthias Pueski

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"

//==============================================================================
/*
    A configurable array of toggle buttons acting as a radio group.
    Exactly one button is active at a time.

    Layers:
      MODULES / ALL  – compact module view with only pins visible
      GUI            – shows the toggle button array

    Pins:
      0 - VALUE OUT  (value of the active button — configurable per-button,
                      default = button index)

    Konfiguration (PropertyView):
      - Buttons (1..16): Anzahl der Buttons im Radio.
      - Labels (CSV):    kommagetrennte Liste, jeder Eintrag wird zum Button-
                         Text. Anzahl der Eintraege uebersteuert "Buttons" —
                         d.h. "On,Off,Auto" setzt automatisch count=3.
      - Values (CSV):    kommagetrennte float-Werte, einer pro Button.
                         Eintraege ueber dem aktuellen Count werden ignoriert,
                         fehlende mit dem Default (Button-Index) aufgefuellt.
*/
class RadioGroupModule : public Module,
                         public juce::Button::Listener
{
public:
    RadioGroupModule(int bufferSize);
    ~RadioGroupModule();

    void paint(juce::Graphics& g) override;
    void process() override;
    void configurePins() override;
    void setCurrentLayer(Layer layer) override;

    Layer getLayer() override { return Layer::GUI; }

    juce::String getCategory() override { return "Controls"; }

    void buttonClicked(juce::Button* b) override;

    void setNumButtons(int n);
    int  getNumButtons() const { return numButtons; }
    int  getSelection()  const { return selection; }
    void setSelection(int s);

    // Per-Button-Konfiguration. Out-of-range Indizes werden still ignoriert.
    void           setButtonLabel(int i, const juce::String& label);
    juce::String   getButtonLabel(int i) const;
    void           setButtonValue(int i, float v);
    float          getButtonValue(int i) const;

    // CSV-Hilfen fuer Persistenz + PropertyView. labelsCsv / valuesCsv stellen
    // den aktuellen Zustand als kommagetrennte Strings dar; setLabelsCsv /
    // setValuesCsv parsen entsprechend. setLabelsCsv passt numButtons an,
    // setValuesCsv laesst numButtons unveraendert.
    juce::String labelsCsv() const;
    juce::String valuesCsv() const;
    void         setLabelsCsv(const juce::String& csv);
    void         setValuesCsv(const juce::String& csv);

    juce::Array<juce::PropertyComponent*>& getProperties() override;
    void                                   createProperties() override;

private:
    void rebuildButtons();
    void updateButtonStates();
    void layoutButtons();
    void refreshLabelsOnButtons();

    int numButtons = 3;
    int selection  = 0;

    juce::OwnedArray<juce::TextButton> buttons;
    std::vector<juce::String>          labels;
    std::vector<float>                 values;

    // PropertyView state. Values bleiben fuer die Lebenszeit des Moduls
    // bestehen, PropertyComponent-Wrapper werden bei jedem getProperties()
    // neu erzeugt (analog zum Pattern in AudioRegion/Region).
    struct CountListener : juce::Value::Listener {
        CountListener(juce::Value& v, RadioGroupModule* o) : owner(o), value(v) { value.addListener(this); }
        void valueChanged(juce::Value& v) override { owner->setNumButtons(v.toString().getIntValue()); }
        RadioGroupModule* owner;
        juce::Value value;
    };
    struct LabelsListener : juce::Value::Listener {
        LabelsListener(juce::Value& v, RadioGroupModule* o) : owner(o), value(v) { value.addListener(this); }
        void valueChanged(juce::Value& v) override { owner->setLabelsCsv(v.toString()); }
        RadioGroupModule* owner;
        juce::Value value;
    };
    struct ValuesListener : juce::Value::Listener {
        ValuesListener(juce::Value& v, RadioGroupModule* o) : owner(o), value(v) { value.addListener(this); }
        void valueChanged(juce::Value& v) override { owner->setValuesCsv(v.toString()); }
        RadioGroupModule* owner;
        juce::Value value;
    };

    juce::Value* countValue  = nullptr;
    juce::Value* labelsValue = nullptr;
    juce::Value* valuesValue = nullptr;

    CountListener*  countListener  = nullptr;
    LabelsListener* labelsListener = nullptr;
    ValuesListener* valuesListener = nullptr;

    juce::PropertyComponent* countProp  = nullptr;
    juce::PropertyComponent* labelsProp = nullptr;
    juce::PropertyComponent* valuesProp = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RadioGroupModule)
};
