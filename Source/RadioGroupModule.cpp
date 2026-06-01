/*
  ==============================================================================

    RadioGroupModule.cpp
    Created: 2026
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "RadioGroupModule.h"
#include "Connection.h"

using juce::TextButton;
using juce::dontSendNotification;

// Compact size used in MODULES / ALL layer
static constexpr int kCompactW = 90;
static constexpr int kCompactH = 40;

// Per-button metrics in GUI layer
static constexpr int kBtnH    = 26;
static constexpr int kBtnGap  = 4;
static constexpr int kPadX    = 6;
static constexpr int kPadY    = 8;
static constexpr int kGuiW    = 120;

static constexpr int kMaxButtons = 16;

//==============================================================================
RadioGroupModule::RadioGroupModule(int bufferSize)
{
    setSize(kCompactW, kCompactH);
    setName("Radio group");
    editable = false;
    prefab   = true;
    this->buffersize = bufferSize;

    rebuildButtons();
    setInterceptsMouseClicks(false, false);
    createProperties();
}

RadioGroupModule::~RadioGroupModule()
{
    if (countValue)  countValue->removeListener(countListener);
    if (labelsValue) labelsValue->removeListener(labelsListener);
    if (valuesValue) valuesValue->removeListener(valuesListener);
    delete countListener;
    delete labelsListener;
    delete valuesListener;
    delete countValue;
    delete labelsValue;
    delete valuesValue;
}

void RadioGroupModule::configurePins()
{
    Pin* out = new Pin(Pin::Type::VALUE);
    out->direction = Pin::Direction::OUT;
    out->setName("Val");
    addPin(Pin::Direction::OUT, out);
}

//==============================================================================
void RadioGroupModule::rebuildButtons()
{
    // labels/values an numButtons angleichen — wachsen mit Default
    // (Label = index, Value = index), schrumpfen verwerfen den Ueberschuss.
    while ((int) labels.size() < numButtons)
        labels.push_back(juce::String((int) labels.size()));
    while ((int) values.size() < numButtons)
        values.push_back((float) values.size());
    if ((int) labels.size() > numButtons) labels.resize(numButtons);
    if ((int) values.size() > numButtons) values.resize(numButtons);

    for (auto* b : buttons)
        removeChildComponent(b);
    buttons.clear();

    for (int i = 0; i < numButtons; ++i)
    {
        auto* btn = new TextButton(labels[(size_t) i]);
        btn->setClickingTogglesState(true);
        btn->setToggleState(i == selection, dontSendNotification);
        btn->addListener(this);
        addChildComponent(btn);   // invisible until GUI layer is active
        buttons.add(btn);
    }
}

void RadioGroupModule::updateButtonStates()
{
    for (int i = 0; i < buttons.size(); ++i)
        buttons[i]->setToggleState(i == selection, dontSendNotification);
}

void RadioGroupModule::refreshLabelsOnButtons()
{
    for (int i = 0; i < buttons.size() && i < (int) labels.size(); ++i)
        buttons[i]->setButtonText(labels[(size_t) i]);
}

void RadioGroupModule::layoutButtons()
{
    int y = kPadY;
    for (auto* b : buttons)
    {
        b->setBounds(kPadX, y, getWidth() - 2 * kPadX, kBtnH);
        y += kBtnH + kBtnGap;
    }
}

//==============================================================================
void RadioGroupModule::setNumButtons(int n)
{
    n = juce::jlimit(1, kMaxButtons, n);
    if (n == numButtons)
        return;
    numButtons = n;
    if (selection >= numButtons)
        selection = 0;
    rebuildButtons();
    // Property-Value resynchronisieren — TextPropertyComponent zeigt sonst
    // den alten Stand, bis der User die Selektion erneuert.
    if (countValue)  countValue->setValue(numButtons);
    if (labelsValue) labelsValue->setValue(labelsCsv());
    if (valuesValue) valuesValue->setValue(valuesCsv());
    setCurrentLayer(currentLayer);   // re-apply layout / visibility
}

void RadioGroupModule::setSelection(int s)
{
    selection = juce::jlimit(0, numButtons - 1, s);
    updateButtonStates();
}

//==============================================================================
void RadioGroupModule::setButtonLabel(int i, const juce::String& label)
{
    if (i < 0 || i >= (int) labels.size()) return;
    labels[(size_t) i] = label;
    if (i < buttons.size())
        buttons[i]->setButtonText(label);
}

juce::String RadioGroupModule::getButtonLabel(int i) const
{
    if (i < 0 || i >= (int) labels.size()) return {};
    return labels[(size_t) i];
}

void RadioGroupModule::setButtonValue(int i, float v)
{
    if (i < 0 || i >= (int) values.size()) return;
    values[(size_t) i] = v;
}

float RadioGroupModule::getButtonValue(int i) const
{
    if (i < 0 || i >= (int) values.size()) return 0.0f;
    return values[(size_t) i];
}

//==============================================================================
juce::String RadioGroupModule::labelsCsv() const
{
    juce::StringArray a;
    for (auto& s : labels) a.add(s);
    return a.joinIntoString(",");
}

juce::String RadioGroupModule::valuesCsv() const
{
    juce::StringArray a;
    for (auto v : values) a.add(juce::String(v, 4));
    return a.joinIntoString(",");
}

void RadioGroupModule::setLabelsCsv(const juce::String& csv)
{
    juce::StringArray parts;
    parts.addTokens(csv, ",", "");
    for (auto& s : parts) s = s.trim();
    // Leere Eingabe ignorieren — sonst wuerde der User durch versehentliches
    // Loeschen des Felds den Modulinhalt verlieren.
    if (parts.size() == 0)
        return;
    const int newCount = juce::jlimit(1, kMaxButtons, parts.size());
    labels.clear();
    for (int i = 0; i < newCount; ++i)
        labels.push_back(parts[i]);
    // numButtons auf neue Count setzen — pad/trim von values uebernimmt
    // rebuildButtons.
    numButtons = newCount;
    if (selection >= numButtons) selection = 0;
    rebuildButtons();
    refreshLabelsOnButtons();
    if (countValue)  countValue->setValue(numButtons);
    if (valuesValue) valuesValue->setValue(valuesCsv());
    setCurrentLayer(currentLayer);
}

void RadioGroupModule::setValuesCsv(const juce::String& csv)
{
    juce::StringArray parts;
    parts.addTokens(csv, ",", "");
    // Auf aktuellem numButtons-Stand bleiben — Werte ueberzaehlig: ignorieren;
    // Werte fehlend: mit Index-Default auffuellen.
    for (int i = 0; i < numButtons; ++i)
    {
        if (i < parts.size())
            values[(size_t) i] = parts[i].trim().getFloatValue();
        else
            values[(size_t) i] = (float) i;
    }
}

//==============================================================================
void RadioGroupModule::buttonClicked(juce::Button* b)
{
    for (int i = 0; i < buttons.size(); ++i)
    {
        if (buttons[i] == b)
        {
            selection = i;
            break;
        }
    }
    // Enforce radio behaviour: exactly one button active
    updateButtonStates();
}

//==============================================================================
void RadioGroupModule::setCurrentLayer(Layer layer)
{
    Module::setCurrentLayer(layer);

    const bool guiActive = (layer == Layer::GUI);

    if (!guiActive)
    {
        // MODULES / ALL layer: compact schematic box, buttons hidden
        setSize(kCompactW, kCompactH);
        for (auto* b : buttons)
            b->setVisible(false);
        setInterceptsMouseClicks(false, false);
    }
    else
    {
        // GUI layer: show buttons stacked vertically
        int h = kPadY + numButtons * (kBtnH + kBtnGap) - kBtnGap + kPadY;
        setSize(kGuiW, h);
        layoutButtons();
        for (auto* b : buttons)
            b->setVisible(true);
        setInterceptsMouseClicks(false, true);  // children (buttons) must receive clicks
    }
}

//==============================================================================
void RadioGroupModule::process()
{
    if (pins.empty()) return;
    const float out = (selection >= 0 && selection < (int) values.size())
                          ? values[(size_t) selection]
                          : (float) selection;
    pins.at(0)->setValue(out);
}

void RadioGroupModule::paint(juce::Graphics& g)
{
    Module::paint(g);
}

//==============================================================================
void RadioGroupModule::createProperties()
{
    countValue  = new juce::Value();
    labelsValue = new juce::Value();
    valuesValue = new juce::Value();

    countValue->setValue(numButtons);
    labelsValue->setValue(labelsCsv());
    valuesValue->setValue(valuesCsv());

    countListener  = new CountListener(*countValue, this);
    labelsListener = new LabelsListener(*labelsValue, this);
    valuesListener = new ValuesListener(*valuesValue, this);
}

juce::Array<juce::PropertyComponent*>& RadioGroupModule::getProperties()
{
    // Aktuellen Stand in die Values pushen — PropertyPanel kann mehrfach
    // refresht werden, ohne dass wir Listener-Ketten verlieren.
    if (countValue)  countValue->setValue(numButtons);
    if (labelsValue) labelsValue->setValue(labelsCsv());
    if (valuesValue) valuesValue->setValue(valuesCsv());

    // Basis-Properties (Name, "Display at root", Is instrument) uebernehmen —
    // ohne den Aufruf fehlte dem RadioGroup-Modul die "Display at root"-Checkbox,
    // sodass es nicht aufs Control-Surface/Root-Level geflaggt werden konnte.
    // Module::getProperties() setzt `properties` frisch auf und fuellt die Basis;
    // die RadioGroup-spezifischen Properties haengen wir danach an.
    Module::getProperties();

    countProp  = new juce::SliderPropertyComponent(*countValue,  "Buttons", 1.0, (double) kMaxButtons, 1.0, 1.0, true);
    labelsProp = new juce::TextPropertyComponent(*labelsValue, "Labels (csv)", 256, false, true);
    valuesProp = new juce::TextPropertyComponent(*valuesValue, "Values (csv)", 256, false, true);

    properties.add(countProp);
    properties.add(labelsProp);
    properties.add(valuesProp);

    return properties;
}
