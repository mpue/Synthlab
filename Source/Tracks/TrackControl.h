/*
  ==============================================================================

    TrackControl.h
    Schmaler Steuer-Strip links neben jedem Track im TrackNavigator.
    Mute / Solo / Pan / Volume / Name + Toggle für die Lane-Automation.

    Wird vom TrackNavigator pro Track angelegt und an x=0 platziert. Der
    eigentliche Track liegt dann ab x=kControlsWidth (siehe TrackNavigator).
    Damit bleibt die komplette Region-Logik (Pixel ↔ Sample-Offset) im
    Track-internen Coord-System unverändert.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Track.h"
#include "AutomationLane.h"
#include "../PluginModule.h"

class TrackControl : public juce::Component,
                     private juce::Button::Listener,
                     private juce::Slider::Listener,
                     private juce::ComboBox::Listener
{
public:
    // Eigene LookAndFeel für das Parameter-Dropdown — die Default-Font des
    // Editor-LookAndFeel ist im Popup so klein, dass die Plugin-Parameternamen
    // praktisch unlesbar sind. Hier nur Schriftgrößen overridden, alles
    // andere bleibt am Parent-Look hängen.
    struct AutoComboLookAndFeel : public juce::LookAndFeel_V4
    {
        juce::Font getComboBoxFont(juce::ComboBox&) override
        {
            return juce::Font(16.0f);
        }
        juce::Font getPopupMenuFont() override
        {
            return juce::Font(18.0f);
        }
        // Mindesthöhe für Popup-Items, sonst können Themes die Items unter
        // die Font-Größe zusammenquetschen.
        void getIdealPopupMenuItemSize(const juce::String& text,
                                       bool isSeparator,
                                       int standardMenuItemHeight,
                                       int& idealWidth,
                                       int& idealHeight) override
        {
            juce::LookAndFeel_V4::getIdealPopupMenuItemSize(text, isSeparator,
                juce::jmax(standardMenuItemHeight, 26),
                idealWidth, idealHeight);
            if (idealHeight < 26) idealHeight = 26;
        }
        int getPopupMenuBorderSize() override { return 6; }
    };

    static AutoComboLookAndFeel& sharedComboLook()
    {
        static AutoComboLookAndFeel instance;
        return instance;
    }

    TrackControl(Track* track) : track(track)
    {
        // Gemeinsame Palette — Track-Farbe als Akzent, dunkler Slate als Off-
        // Hintergrund, helle Texte. Wird unten an jedes Sub-Control verteilt.
        const juce::Colour base    = track != nullptr ? track->getTrackColour()
                                                      : juce::Colour(0xff5a8fff);
        const juce::Colour accent  = base.withMultipliedSaturation(1.05f)
                                         .withMultipliedBrightness(1.10f);
        const juce::Colour btnOff  = juce::Colour(0xff1c1f25);
        const juce::Colour trackBg = juce::Colour(0xff15171c);
        const juce::Colour textHi  = juce::Colour(0xffe8e9ec);
        const juce::Colour textLo  = juce::Colour(0xffa8acb4);

        nameLabel.setText(track->getName(), juce::dontSendNotification);
        nameLabel.setFont(juce::Font(12.0f, juce::Font::bold));
        nameLabel.setColour(juce::Label::textColourId, textHi);
        nameLabel.setJustificationType(juce::Justification::centredLeft);
        nameLabel.setInterceptsMouseClicks(false, false);
        addAndMakeVisible(nameLabel);

        auto styleToggle = [&](juce::TextButton& b, juce::Colour onColour)
        {
            b.setClickingTogglesState(true);
            b.setColour(juce::TextButton::buttonColourId,    btnOff);
            b.setColour(juce::TextButton::buttonOnColourId,  onColour);
            b.setColour(juce::TextButton::textColourOffId,   textLo);
            b.setColour(juce::TextButton::textColourOnId,    juce::Colours::white);
            b.addListener(this);
            addAndMakeVisible(b);
        };

        muteButton.setButtonText("M");
        muteButton.setToggleState(track->isMute(), juce::dontSendNotification);
        styleToggle(muteButton, juce::Colour(0xffd34d4d));

        soloButton.setButtonText("S");
        soloButton.setToggleState(track->isSolo(), juce::dontSendNotification);
        styleToggle(soloButton, juce::Colour(0xffe0a64a));

        // Automation-Toggle (kleiner Button "A"). Schaltet das Lane-Overlay am
        // Track ein/aus. Erst wenn das Overlay an ist, blenden wir die ComboBox
        // mit der Parameter-Auswahl ein.
        autoButton.setButtonText("A");
        autoButton.setToggleState(track != nullptr && track->isAutomationVisible(),
                                  juce::dontSendNotification);
        styleToggle(autoButton, juce::Colour(0xfff0c060));

        auto styleSlider = [&](juce::Slider& s, juce::Colour fill)
        {
            s.setSliderStyle(juce::Slider::LinearHorizontal);
            s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            s.setColour(juce::Slider::backgroundColourId, trackBg);
            s.setColour(juce::Slider::trackColourId,      fill);
            s.setColour(juce::Slider::thumbColourId,      juce::Colours::white);
            s.addListener(this);
            addAndMakeVisible(s);
        };

        panSlider.setRange(-1.0, 1.0, 0.01);
        panSlider.setDoubleClickReturnValue(true, 0.0);
        panSlider.setValue(track->getPan(), juce::dontSendNotification);
        // Pan ist symmetrisch um 0 — die Fill-Bar von links wuerde irrefuehren,
        // also Fill und Hintergrund gleich faerben (nur der Thumb zeigt Wert).
        styleSlider(panSlider, trackBg);

        volSlider.setRange(0.0, 1.5, 0.01);
        volSlider.setDoubleClickReturnValue(true, 1.0);
        volSlider.setValue(track->getVolume(), juce::dontSendNotification);
        styleSlider(volSlider, accent);

        panLabel.setText("Pan", juce::dontSendNotification);
        panLabel.setFont(juce::Font(10.5f, juce::Font::bold));
        panLabel.setColour(juce::Label::textColourId, textLo);
        panLabel.setInterceptsMouseClicks(false, false);
        addAndMakeVisible(panLabel);

        volLabel.setText("Vol", juce::dontSendNotification);
        volLabel.setFont(juce::Font(10.5f, juce::Font::bold));
        volLabel.setColour(juce::Label::textColourId, textLo);
        volLabel.setInterceptsMouseClicks(false, false);
        addAndMakeVisible(volLabel);

        auto styleCombo = [&](juce::ComboBox& c)
        {
            c.setLookAndFeel(&sharedComboLook());
            c.setColour(juce::ComboBox::backgroundColourId, btnOff);
            c.setColour(juce::ComboBox::outlineColourId,    accent.withAlpha(0.55f));
            c.setColour(juce::ComboBox::textColourId,       textHi);
            c.setColour(juce::ComboBox::arrowColourId,      accent);
            c.addListener(this);
            addChildComponent(c); // unsichtbar, bis Automation aktiv ist
        };

        pluginBox.setTextWhenNothingSelected("Plugin ");
        styleCombo(pluginBox);

        paramBox.setTextWhenNothingSelected("Parameter ");
        styleCombo(paramBox);
    }

    ~TrackControl() override
    {
        muteButton.removeListener(this);
        soloButton.removeListener(this);
        autoButton.removeListener(this);
        panSlider.removeListener(this);
        volSlider.removeListener(this);
        pluginBox.removeListener(this);
        pluginBox.setLookAndFeel(nullptr);
        paramBox.removeListener(this);
        paramBox.setLookAndFeel(nullptr);
    }

    void paint(juce::Graphics& g) override
    {
        // Hintergrund: dezenter Tint der Track-Farbe — passt zur Track-Reihe
        // visuell zusammen, ohne dass die Buttons drauf untergehen.
        const juce::Colour base = track != nullptr ? track->getTrackColour()
                                                   : juce::Colour(0xff2a2a2a);
        const juce::Colour bg = base.withMultipliedSaturation(0.45f)
                                    .withMultipliedBrightness(0.30f);
        g.fillAll(bg);

        // Farbiger 4-px-Streifen links als Track-Marker (analog zum Track selbst).
        g.setColour(base);
        g.fillRect(0, 0, 4, getHeight());

        // Trennlinie zum Track rechts daneben.
        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.fillRect(getWidth() - 1, 0, 1, getHeight());
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced(6, 4);
        r.removeFromLeft(4); // Platz für den farbigen Marker-Streifen

        nameLabel.setBounds(r.removeFromTop(16));
        r.removeFromTop(2);

        // M / S / A Buttons + Pan-Slider in einer Reihe
        auto row1 = r.removeFromTop(18);
        muteButton.setBounds(row1.removeFromLeft(20));
        row1.removeFromLeft(2);
        soloButton.setBounds(row1.removeFromLeft(20));
        row1.removeFromLeft(2);
        autoButton.setBounds(row1.removeFromLeft(20));
        row1.removeFromLeft(6);
        panLabel.setBounds(row1.removeFromLeft(22));
        panSlider.setBounds(row1);

        r.removeFromTop(3);

        auto row2 = r.removeFromTop(16);
        row2.removeFromLeft(2);
        volLabel.setBounds(row2.removeFromLeft(22));
        volSlider.setBounds(row2);

        r.removeFromTop(3);

        // ComboBoxen unter den Slidern. Nur sichtbar bei aktiver Automation.
        if (pluginBox.isVisible())
        {
            pluginBox.setBounds(r.removeFromTop(26));
            r.removeFromTop(2);
        }
        if (paramBox.isVisible())
            paramBox.setBounds(r.removeFromTop(26));
    }

    // Vom Navigator aufgerufen, wenn die Werte am Track von außen geändert
    // wurden (z.B. via Property-Panel). Hält UI und Track in Sync.
    void syncFromTrack()
    {
        if (track == nullptr) return;
        nameLabel.setText(track->getName(), juce::dontSendNotification);
        muteButton.setToggleState(track->isMute(), juce::dontSendNotification);
        soloButton.setToggleState(track->isSolo(), juce::dontSendNotification);
        autoButton.setToggleState(track->isAutomationVisible(), juce::dontSendNotification);
        panSlider.setValue(track->getPan(), juce::dontSendNotification);
        volSlider.setValue(track->getVolume(), juce::dontSendNotification);

        refreshParamBoxVisibility();
        repaint();
    }

    Track* getTrack() const { return track; }

private:
    void refreshParamBoxVisibility()
    {
        const bool show = track != nullptr && track->isAutomationVisible();
        if (show)
        {
            populatePluginBox();
            populateParamBox();
            pluginBox.setVisible(true);
            paramBox.setVisible(true);
        }
        else
        {
            pluginBox.setVisible(false);
            paramBox.setVisible(false);
        }
        resized();
    }

    // Fuellt die Plugin-ComboBox aus track->getAutomationTargets(): Instrument
    // (falls vorhanden) gefolgt von den belegten FX-Slots. Wenn keiner
    // existiert, zeigt die Box "(kein Plugin)" und Parameter-Box ist leer.
    void populatePluginBox()
    {
        pluginBox.clear(juce::dontSendNotification);
        currentPluginTargets.clear();
        if (track == nullptr) return;

        currentPluginTargets = track->getAutomationTargets();
        if (currentPluginTargets.empty())
        {
            pluginBox.addItem("(kein Plugin)", 1);
            pluginBox.setSelectedId(0, juce::dontSendNotification);
            pluginBox.setEnabled(false);
            return;
        }
        pluginBox.setEnabled(true);
        for (size_t i = 0; i < currentPluginTargets.size(); ++i)
        {
            const auto& slot = currentPluginTargets[i].first;
            auto* pm = currentPluginTargets[i].second;
            juce::String label = (slot == juce::String("instr"))
                ? juce::String("Instrument")
                : juce::String("FX ") + juce::String(slot.substring(2).getIntValue() + 1);
            label += "  —  " + (pm != nullptr ? pm->getPluginName() : juce::String("?"));
            pluginBox.addItem(label, (int) i + 1);
        }

        // Aktuelle Auswahl wiederherstellen — wenn der Track sich an einen
        // Slot erinnert, den wir kennen, vorselektieren.
        const auto curSlot = track->getAutomationCurrentSlot();
        for (size_t i = 0; i < currentPluginTargets.size(); ++i)
            if (currentPluginTargets[i].first == curSlot)
            {
                pluginBox.setSelectedId((int) i + 1, juce::dontSendNotification);
                break;
            }
        if (pluginBox.getSelectedId() == 0)
            pluginBox.setSelectedId(1, juce::dontSendNotification);
    }

    // Fuellt die Parameter-ComboBox fuer das aktuell in pluginBox gewaehlte
    // Plugin. Verwendet currentPluginTargets[selectedId-1].second.
    void populateParamBox()
    {
        paramBox.clear(juce::dontSendNotification);
        if (track == nullptr) return;

        const int sel = pluginBox.getSelectedId();
        if (sel <= 0 || sel > (int) currentPluginTargets.size())
        {
            paramBox.addItem("(kein Plugin)", 1);
            paramBox.setSelectedId(0, juce::dontSendNotification);
            paramBox.setEnabled(false);
            return;
        }

        auto* pm = currentPluginTargets[(size_t)(sel - 1)].second;
        if (pm == nullptr || pm->getPlugin() == nullptr)
        {
            paramBox.addItem("(kein Plugin)", 1);
            paramBox.setSelectedId(0, juce::dontSendNotification);
            paramBox.setEnabled(false);
            return;
        }

        paramBox.setEnabled(true);
        const auto& params = pm->getPlugin()->getParameters();
        int id = 1;
        for (auto* p : params)
        {
            if (p == nullptr) continue;
            if (!p->isAutomatable()) { ++id; continue; }
            paramBox.addItem(p->getName(40), id);
            ++id;
        }
    }

    void buttonClicked(juce::Button* b) override
    {
        if (track == nullptr) return;
        if (b == &muteButton) track->setMute(muteButton.getToggleState());
        else if (b == &soloButton) track->setSolo(soloButton.getToggleState());
        else if (b == &autoButton)
        {
            track->setAutomationVisible(autoButton.getToggleState());
            refreshParamBoxVisibility();
        }
    }

    void sliderValueChanged(juce::Slider* s) override
    {
        if (track == nullptr) return;
        if (s == &panSlider) track->setPan((float)panSlider.getValue());
        else if (s == &volSlider) track->setVolume((float)volSlider.getValue());
    }

    void comboBoxChanged(juce::ComboBox* c) override
    {
        if (track == nullptr) return;
        if (c == &pluginBox)
        {
            populateParamBox();
            return;
        }
        if (c != &paramBox) return;

        const int paramSel  = paramBox.getSelectedId();  // 1-based
        const int pluginSel = pluginBox.getSelectedId(); // 1-based
        if (paramSel <= 0 || pluginSel <= 0
            || pluginSel > (int) currentPluginTargets.size()) return;

        const auto& slot = currentPluginTargets[(size_t)(pluginSel - 1)].first;
        auto* pm = currentPluginTargets[(size_t)(pluginSel - 1)].second;
        if (pm == nullptr || pm->getPlugin() == nullptr) return;

        const auto& params = pm->getPlugin()->getParameters();
        const int idxInPlugin = paramSel - 1;
        if (idxInPlugin < 0 || idxInPlugin >= params.size()) return;

        auto* p = params[idxInPlugin];
        if (p == nullptr) return;

        // Plugin-eigene paramId aufloesen — gleiche Logik wie Track::localParamId().
        juce::String localId;
        if (auto* withId = dynamic_cast<juce::AudioProcessorParameterWithID*>(p))
            localId = withId->paramID;
        else
            localId = juce::String(idxInPlugin) + ":" + p->getName(64);

        track->setAutomationTarget(slot, localId);
    }

    Track* track = nullptr;

    juce::Label       nameLabel;
    juce::TextButton  muteButton;
    juce::TextButton  soloButton;
    juce::TextButton  autoButton;
    juce::Slider      panSlider, volSlider;
    juce::Label       panLabel, volLabel;
    juce::ComboBox    pluginBox;  // Instrument + FX-Slots
    juce::ComboBox    paramBox;   // Parameter im gewaehlten Plugin

    // Spiegel der Plugin-Targets, die in pluginBox gelistet sind, in der
    // gleichen Reihenfolge. Wird in populatePluginBox() gefuellt.
    std::vector<std::pair<juce::String, PluginModule*>> currentPluginTargets;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackControl)
};
