/*
  ==============================================================================

    PianoRollEditor.h

    Wrapper um PianoRollComponent mit einer Toolbar oben drueber:
    Tool-Auswahl (Select/Pencil/Eraser/Velocity/Split), Grid-Aufloesung,
    Skala und Snap-Toggle. Die eigentliche Roll-Logik bleibt in
    PianoRollComponent — dieser Wrapper ist reines UI/Layout und ruft die
    bereits vorhandenen Setter auf.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "PianoRollComponent.h"

class PianoRollEditor : public juce::Component
{
public:
    PianoRollEditor(MidiRegion* region, double sampleRate)
        : roll(region, sampleRate)
    {
        addAndMakeVisible(roll);

        // Tool-Buttons --------------------------------------------------------
        // Rectangular GroupId == 1 sodass nur einer aktiv aussehen kann; die
        // tatsaechliche Aktivierung kommt vom PianoRollComponent::onToolChanged
        // zurueck, damit Tasten 1..5 die Buttons synchron halten.
        auto setupToolButton = [this](juce::TextButton& b, juce::String label,
                                      PianoRollComponent::Tool t)
        {
            b.setButtonText(label);
            b.setClickingTogglesState(true);
            b.setRadioGroupId(1);
            b.setColour(juce::TextButton::buttonColourId,    juce::Colour(0xff2c2c2c));
            b.setColour(juce::TextButton::buttonOnColourId,  juce::Colour(0xff4a8edc));
            b.setColour(juce::TextButton::textColourOffId,   juce::Colours::lightgrey);
            b.setColour(juce::TextButton::textColourOnId,    juce::Colours::white);
            b.onClick = [this, t]() { roll.setCurrentTool(t); };
            addAndMakeVisible(b);
        };

        setupToolButton(toolSelect,   "Select",   PianoRollComponent::Tool::SELECT);
        setupToolButton(toolPencil,   "Pencil",   PianoRollComponent::Tool::PENCIL);
        setupToolButton(toolEraser,   "Eraser",   PianoRollComponent::Tool::ERASER);
        setupToolButton(toolVelocity, "Velocity", PianoRollComponent::Tool::VELOCITY);
        setupToolButton(toolSplit,    "Split",    PianoRollComponent::Tool::SPLIT);

        // Grid-Auswahl --------------------------------------------------------
        gridLabel.setText("Grid:", juce::dontSendNotification);
        gridLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
        gridLabel.setJustificationType(juce::Justification::centredRight);
        addAndMakeVisible(gridLabel);

        gridCombo.addItem("1/1 (whole)",   1);
        gridCombo.addItem("1/2 (half)",    2);
        gridCombo.addItem("1/4",           4);
        gridCombo.addItem("1/8",           8);
        gridCombo.addItem("1/16",         16);
        gridCombo.addItem("1/32",         32);
        gridCombo.setSelectedId(roll.getGridDenominator(), juce::dontSendNotification);
        gridCombo.onChange = [this]() {
            const int denom = gridCombo.getSelectedId();
            if (denom > 0) roll.setGridResolution(denom);
        };
        addAndMakeVisible(gridCombo);

        // Skala ---------------------------------------------------------------
        scaleLabel.setText("Scale:", juce::dontSendNotification);
        scaleLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
        scaleLabel.setJustificationType(juce::Justification::centredRight);
        addAndMakeVisible(scaleLabel);

        // ID 1 = chromatic (kein Snap); 2..n = vordefinierte Skalen. Die
        // Intervalle stehen unten als statisches Lookup, damit Hinzufuegen
        // einer Skala nur eine Zeile braucht.
        scaleCombo.addItem("Chromatic",     1);
        scaleCombo.addItem("C Major",       2);
        scaleCombo.addItem("A Minor",       3);
        scaleCombo.addItem("C Minor",       4);
        scaleCombo.addItem("Major Pentatonic", 5);
        scaleCombo.addItem("Minor Pentatonic", 6);
        scaleCombo.addItem("Blues",         7);
        scaleCombo.addItem("Dorian",        8);
        scaleCombo.addItem("Mixolydian",    9);
        scaleCombo.setSelectedId(2, juce::dontSendNotification); // matcht Default
        applySelectedScale();
        scaleCombo.onChange = [this]() { applySelectedScale(); };
        addAndMakeVisible(scaleCombo);

        // Snap-Toggle ---------------------------------------------------------
        snapToggle.setButtonText("Snap");
        snapToggle.setToggleState(roll.isSnapToGrid(), juce::dontSendNotification);
        snapToggle.setColour(juce::ToggleButton::textColourId, juce::Colours::lightgrey);
        snapToggle.onClick = [this]() { roll.setSnapToGrid(snapToggle.getToggleState()); };
        addAndMakeVisible(snapToggle);

        // Notes-Helper-Menue --------------------------------------------------
        // Sammelpunkt fuer Aktionen, die nur fuer die aktuelle Auswahl Sinn
        // haben (Quantize / Humanize / Tempo). Items werden disabled gerendert
        // wenn keine Notes selektiert sind.
        notesMenuButton.setButtonText("Notes  v");
        notesMenuButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2c2c2c));
        notesMenuButton.setColour(juce::TextButton::textColourOffId, juce::Colours::lightgrey);
        notesMenuButton.onClick = [this]() { showNotesMenu(); };
        addAndMakeVisible(notesMenuButton);

        // Initialer Toggle-Zustand der Tool-Buttons setzen + Sync via Callback,
        // damit Tasten 1..5 die Toolbar mit aktualisieren.
        syncToolButtons(roll.getCurrentTool());
        roll.onToolChanged = [this](PianoRollComponent::Tool t) { syncToolButtons(t); };
    }

    PianoRollComponent& getRoll() { return roll; }

    void resized() override
    {
        auto bounds = getLocalBounds();
        auto bar = bounds.removeFromTop(kToolbarHeight);

        // Hintergrund der Toolbar zeichnen wir in paint(); hier nur Layout.
        bar.reduce(6, 4);

        auto layoutButton = [&](juce::Component& c, int w) {
            c.setBounds(bar.removeFromLeft(w));
            bar.removeFromLeft(4);
        };

        layoutButton(toolSelect,   60);
        layoutButton(toolPencil,   60);
        layoutButton(toolEraser,   60);
        layoutButton(toolVelocity, 70);
        layoutButton(toolSplit,    50);

        bar.removeFromLeft(16); // Separator-Abstand
        layoutButton(gridLabel,    40);
        layoutButton(gridCombo,   100);

        bar.removeFromLeft(12);
        layoutButton(scaleLabel,   45);
        layoutButton(scaleCombo,  150);

        bar.removeFromLeft(12);
        layoutButton(snapToggle,   60);

        bar.removeFromLeft(12);
        layoutButton(notesMenuButton, 95);

        roll.setBounds(bounds);
    }

    void paint(juce::Graphics& g) override
    {
        // Toolbar-Hintergrund — dezent dunkler als der Roll-Body.
        g.setColour(juce::Colour(0xff1a1a1a));
        g.fillRect(0, 0, getWidth(), kToolbarHeight);
        g.setColour(juce::Colour(0xff333333));
        g.drawHorizontalLine(kToolbarHeight - 1, 0.0f, (float) getWidth());
    }

private:
    static constexpr int kToolbarHeight = 32;

    PianoRollComponent roll;

    juce::TextButton  toolSelect, toolPencil, toolEraser, toolVelocity, toolSplit;
    juce::Label       gridLabel, scaleLabel;
    juce::ComboBox    gridCombo, scaleCombo;
    juce::ToggleButton snapToggle;
    juce::TextButton  notesMenuButton;

    void syncToolButtons(PianoRollComponent::Tool t)
    {
        // Toggle-State setzen ohne erneut onClick zu feuern (sendNotification
        // = dontSendNotification), sonst Endlosschleife: setToggleState ->
        // onClick -> setCurrentTool -> onToolChanged -> syncToolButtons.
        toolSelect  .setToggleState(t == PianoRollComponent::Tool::SELECT,   juce::dontSendNotification);
        toolPencil  .setToggleState(t == PianoRollComponent::Tool::PENCIL,   juce::dontSendNotification);
        toolEraser  .setToggleState(t == PianoRollComponent::Tool::ERASER,   juce::dontSendNotification);
        toolVelocity.setToggleState(t == PianoRollComponent::Tool::VELOCITY, juce::dontSendNotification);
        toolSplit   .setToggleState(t == PianoRollComponent::Tool::SPLIT,    juce::dontSendNotification);
    }

    void showNotesMenu()
    {
        // Items wirken jeweils auf die selektierten Notes — wenn nichts
        // selektiert ist, gibt's nichts zu tun, also disabled rendern.
        const bool haveSel       = roll.getSelectedNoteCount() > 0;
        const bool haveSelAndScale = haveSel && !roll.getCustomScale().empty();

        juce::PopupMenu m;
        m.addItem(1, "Quantize to grid",       haveSel);
        m.addItem(2, "Humanize",               haveSel);
        m.addSeparator();
        m.addItem(3, "Half Tempo (2x length)",   haveSel);
        m.addItem(4, "Double Tempo (1/2 length)", haveSel);
        m.addSeparator();
        // Bei "Chromatic" (leere Skala) gibt's keine sinnvolle Akkord-
        // Konstruktion — disabled rendern und im Tooltip-Text auf das
        // Skala-Combo verweisen.
        m.addItem(5, "Chords from current scale", haveSelAndScale);

        m.showMenuAsync(juce::PopupMenu::Options()
                            .withTargetComponent(&notesMenuButton),
            [this](int result) {
                switch (result) {
                    case 1: roll.quantizeSelected(roll.getGridDenominator()); break;
                    case 2: roll.humanizeSelected();                          break;
                    case 3: roll.stretchSelectedTempo(2.0f);                  break;
                    case 4: roll.stretchSelectedTempo(0.5f);                  break;
                    case 5: roll.buildChordsFromScaleForSelection();          break;
                    default: break;
                }
            });
    }

    void applySelectedScale()
    {
        const int id = scaleCombo.getSelectedId();
        std::vector<int> scale;
        switch (id) {
            case 1: scale = {};                         break; // chromatic
            case 2: scale = {0, 2, 4, 5, 7, 9, 11};     break; // C major / Ionian
            case 3: scale = {9, 11, 0, 2, 4, 5, 7};     break; // A minor / Aeolian (rooted at A)
            case 4: scale = {0, 2, 3, 5, 7, 8, 10};     break; // C minor (natural)
            case 5: scale = {0, 2, 4, 7, 9};            break; // Major pentatonic (C)
            case 6: scale = {0, 3, 5, 7, 10};           break; // Minor pentatonic (C)
            case 7: scale = {0, 3, 5, 6, 7, 10};        break; // Blues (C)
            case 8: scale = {0, 2, 3, 5, 7, 9, 10};     break; // Dorian (C)
            case 9: scale = {0, 2, 4, 5, 7, 9, 10};     break; // Mixolydian (C)
            default: scale = {};                        break;
        }
        // Auf Tonklassen normalisieren (mod 12) und sortieren — snapPitchToScale
        // erwartet Werte 0..11.
        for (auto& n : scale) n = ((n % 12) + 12) % 12;
        std::sort(scale.begin(), scale.end());
        roll.setCustomScale(scale);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoRollEditor)
};
