/*
  ==============================================================================
    ActivationDialog.h
    Author:  Matthias Pueski

    Modaler Aktivierungs-Dialog beim Start, wenn die App nicht aktiviert ist.
    Felder fuer E-Mail + License-Key, Aktivierung laeuft im Hintergrund-Thread,
    damit die UI nicht blockiert. Fehlermeldungen werden nutzerfreundlich auf
    Deutsch gemappt.
  ==============================================================================
*/

#pragma once

#include "../../JuceLibraryCode/JuceHeader.h"
#include "LicenseManager.h"

class ActivationWindow;

class ActivationDialog : public juce::Component
{
public:
    /** Oeffnet den Aktivierungs-Dialog als eigenes Fenster.
        @param onActivated    wird auf dem Message-Thread aufgerufen, sobald die
                              Aktivierung erfolgreich war (dort z.B. das
                              Hauptfenster erzeugen).
        @param reactivation   true, wenn der Grund ein Machine-Mismatch ist
                              (anderer einleitender Text). */
    static void launch (std::function<void()> onActivated, bool reactivation);

    ActivationDialog (ActivationWindow& owner, bool reactivation);
    ~ActivationDialog() override;

    void resized() override;
    void paint (juce::Graphics&) override;

private:
    void activateClicked();
    void serverClicked();
    void onActivationFinished (ActivationResult result);
    void setBusy (bool busy);

    static juce::String messageForResult (ActivationResult result);

    ActivationWindow& owner;

    juce::Label        headline;
    juce::Label        emailLabel  { {}, "Email:" };
    juce::Label        keyLabel    { {}, "License key:" };
    juce::TextEditor   emailEditor;
    juce::TextEditor   keyEditor;
    juce::TextButton   activateButton { "Activate" };
    juce::TextButton   serverButton   { "Server..." };
    juce::TextButton   quitButton     { "Quit" };
    juce::Label        statusLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ActivationDialog)
};

//==============================================================================
/** Tragendes Fenster. closeButtonPressed() / „Beenden" beenden die App, da ohne
    Aktivierung kein Hauptfenster existiert. Bei Erfolg ruft der Dialog
    activationSucceeded(), das onActivated() ausfuehrt und sich selbst schliesst. */
class ActivationWindow : public juce::DocumentWindow
{
public:
    ActivationWindow (std::function<void()> onActivated, bool reactivation);

    void closeButtonPressed() override;
    void activationSucceeded();
    void quitApp();

private:
    std::function<void()> onActivatedCallback;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ActivationWindow)
};
