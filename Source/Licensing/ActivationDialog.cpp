/*
  ==============================================================================
    ActivationDialog.cpp
    Author:  Matthias Pueski
  ==============================================================================
*/

#include "ActivationDialog.h"
#include "../Project/Project.h"
#include "../Settings.h"
#include <thread>

//==============================================================================
ActivationDialog::ActivationDialog (ActivationWindow& ownerWindow, bool reactivation)
    : owner (ownerWindow)
{
    headline.setText (reactivation
                          ? "This device has not been activated for Synthlab yet."
                          : "Please activate your Synthlab license.",
                      juce::dontSendNotification);
    headline.setJustificationType (juce::Justification::centredLeft);
    headline.setFont (juce::Font (16.0f, juce::Font::bold));
    addAndMakeVisible (headline);

    addAndMakeVisible (emailLabel);
    addAndMakeVisible (keyLabel);

    emailEditor.setTextToShowWhenEmpty ("name@example.com", juce::Colours::grey);
    keyEditor.setTextToShowWhenEmpty ("XXXX-XXXX-XXXX-XXXX", juce::Colours::grey);
    addAndMakeVisible (emailEditor);
    addAndMakeVisible (keyEditor);

    activateButton.onClick = [this] { activateClicked(); };
    serverButton.onClick   = [this] { serverClicked(); };
    quitButton.onClick     = [this] { owner.quitApp(); };
    addAndMakeVisible (activateButton);
    addAndMakeVisible (serverButton);
    addAndMakeVisible (quitButton);

    statusLabel.setJustificationType (juce::Justification::centredLeft);
    statusLabel.setMinimumHorizontalScale (1.0f);
    addAndMakeVisible (statusLabel);

    setSize (460, 240);
}

ActivationDialog::~ActivationDialog() {}

void ActivationDialog::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void ActivationDialog::resized()
{
    auto area = getLocalBounds().reduced (16);

    headline.setBounds (area.removeFromTop (28));
    area.removeFromTop (12);

    auto row = [&area] (int h) { auto r = area.removeFromTop (h); area.removeFromTop (8); return r; };

    {
        auto r = row (26);
        emailLabel.setBounds (r.removeFromLeft (96));
        emailEditor.setBounds (r);
    }
    {
        auto r = row (26);
        keyLabel.setBounds (r.removeFromLeft (96));
        keyEditor.setBounds (r);
    }

    statusLabel.setBounds (row (40));

    auto buttons = area.removeFromBottom (30);
    activateButton.setBounds (buttons.removeFromRight (130));
    buttons.removeFromRight (8);
    quitButton.setBounds (buttons.removeFromRight (110));
    serverButton.setBounds (buttons.removeFromLeft (110));
}

//==============================================================================
void ActivationDialog::setBusy (bool busy)
{
    activateButton.setEnabled (! busy);
    quitButton.setEnabled (! busy);
    emailEditor.setEnabled (! busy);
    keyEditor.setEnabled (! busy);
}

void ActivationDialog::activateClicked()
{
    const juce::String email = emailEditor.getText().trim();
    const juce::String key   = keyEditor.getText().trim();

    if (email.isEmpty() || key.isEmpty())
    {
        statusLabel.setColour (juce::Label::textColourId, juce::Colours::orange);
        statusLabel.setText ("Please enter your email and license key.", juce::dontSendNotification);
        return;
    }

    setBusy (true);
    statusLabel.setColour (juce::Label::textColourId, juce::Colours::lightgrey);
    statusLabel.setText ("Activating...", juce::dontSendNotification);

    // Aktivierung (Netzwerk) im Hintergrund — UI nicht blockieren. Ergebnis
    // wird ueber eine SafePointer-gesicherte callAsync zurueck auf den
    // Message-Thread gebracht.
    juce::Component::SafePointer<ActivationDialog> safe (this);
    std::thread ([safe, key, email]()
    {
        const ActivationResult result = LicenseManager::getInstance()->activate (key, email);
        juce::MessageManager::callAsync ([safe, result]()
        {
            if (safe != nullptr)
                safe->onActivationFinished (result);
        });
    }).detach();
}

void ActivationDialog::serverClicked()
{
    // Erlaubt das Umstellen des Lizenzservers schon beim Erststart, bevor das
    // Hauptfenster (und damit das Menue "License Server...") existiert.
    auto* w = new juce::AlertWindow (
        "License Server",
        "Base URL of the license server (without the trailing /activate). "
        "Leave empty to use the built-in default.",
        juce::AlertWindow::NoIcon);

    w->setLookAndFeel (&getLookAndFeel());
    w->addTextEditor ("url", Settings::getInstance()->getLicenseServerUrl(), "URL:");
    w->addButton ("OK", 1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    w->enterModalState (true,
        juce::ModalCallbackFunction::create ([w](int result)
        {
            if (result == 1)
                Settings::getInstance()->setLicenseServerUrl (w->getTextEditorContents ("url"));
        }),
        true /* deleteWhenDismissed */);
}

void ActivationDialog::onActivationFinished (ActivationResult result)
{
    if (result == ActivationResult::Success)
    {
        statusLabel.setColour (juce::Label::textColourId, juce::Colours::lightgreen);
        statusLabel.setText ("Activation successful.", juce::dontSendNotification);
        owner.activationSucceeded();
        return;
    }

    setBusy (false);
    statusLabel.setColour (juce::Label::textColourId, juce::Colours::tomato);
    statusLabel.setText (messageForResult (result), juce::dontSendNotification);
}

juce::String ActivationDialog::messageForResult (ActivationResult result)
{
    switch (result)
    {
        case ActivationResult::InvalidKey:
            return "Invalid license key or email.";
        case ActivationResult::Refunded:
            return "This license was refunded and is no longer valid.";
        case ActivationResult::LimitReached:
            return "Maximum number of devices reached. Please deactivate a device "
                   "or contact support.";
        case ActivationResult::NetworkError:
            return "Could not reach the license server. Please check your internet connection.";
        case ActivationResult::ServerError:
            return "Unexpected server response. Please try again later.";
        case ActivationResult::SodiumError:
            return "Could not initialize cryptography.";
        case ActivationResult::Success:
        case ActivationResult::Unknown:
        default:
            return "Activation failed. Please contact support.";
    }
}

//==============================================================================
void ActivationDialog::launch (std::function<void()> onActivated, bool reactivation)
{
    // Das Fenster besitzt sich selbst (setUsingNativeTitleBar etc. analog zum
    // MainWindow). Es wird in activationSucceeded()/quitApp() abgeraeumt.
    new ActivationWindow (std::move (onActivated), reactivation);
}

//==============================================================================
ActivationWindow::ActivationWindow (std::function<void()> onActivated, bool reactivation)
    : juce::DocumentWindow ("Synthlab - Activation",
                            juce::Colours::darkgrey,
                            juce::DocumentWindow::closeButton),
      onActivatedCallback (std::move (onActivated))
{
    setLookAndFeel (Project::getInstance()->getLookAndFeel());
    setUsingNativeTitleBar (false);
    setContentOwned (new ActivationDialog (*this, reactivation), true);
    setResizable (false, false);
    centreWithSize (getWidth(), getHeight());

    juce::MessageManager::callAsync ([this]()
    {
        setVisible (true);
        toFront (true);
    });
}

void ActivationWindow::closeButtonPressed()
{
    quitApp();
}

void ActivationWindow::quitApp()
{
    setLookAndFeel (nullptr);
    juce::JUCEApplication::getInstance()->systemRequestedQuit();
}

void ActivationWindow::activationSucceeded()
{
    if (onActivatedCallback)
        onActivatedCallback();

    // Selbst-Abbau nach dem aktuellen Callback-Zyklus.
    setLookAndFeel (nullptr);
    juce::MessageManager::callAsync ([this]() { delete this; });
}
