/*
  ==============================================================================

    SplashScreenComponent.h

    Blender-Style Splash, der beim Start ueber dem Hauptfenster liegt und
    erst auf Klick (Continue / ESC / Klick aufs Bild) verschwindet. Bild
    kommt aus BinaryData::splash_png (Images/splash.png im Projucer).

    Persistenter Toggle "Don't show on startup" haengt am Settings-Singleton
    (settings.xml), parallel zum Toggle im Audio-Settings-Dialog.

    Anzeige via DialogWindow::launchAsync — modal (blockiert Eingaben am
    Hauptfenster), aber non-blocking fuer den Code, damit initialise()
    nicht in einer Nested-Message-Loop sitzt.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Settings.h"

class SplashScreenComponent  : public juce::Component
{
public:
    SplashScreenComponent()
    {
        image = juce::ImageCache::getFromMemory (BinaryData::splash_png,
                                                 BinaryData::splash_pngSize);

        continueButton.setButtonText ("Continue");
        continueButton.onClick = [this] { closeDialog(); };
        addAndMakeVisible (continueButton);

        dontShowToggle.setButtonText ("Don't show on startup");
        dontShowToggle.setToggleState (! Settings::getInstance()->getShowSplashOnStartup(),
                                       juce::dontSendNotification);
        dontShowToggle.setColour (juce::ToggleButton::textColourId, juce::Colours::white);
        dontShowToggle.onClick = [this]
        {
            // Toggle invertieren — "Don't show" == true -> Splash aus.
            Settings::getInstance()->setShowSplashOnStartup (! dontShowToggle.getToggleState());
        };
        addAndMakeVisible (dontShowToggle);

        // Groesse: Bild auf max. 800px Breite skalieren, plus Footer fuer Buttons.
        constexpr int kMaxWidth   = 800;
        constexpr int kFooterH    = 56;

        int w = 720, h = 480;
        if (image.isValid())
        {
            const double scale = juce::jmin (1.0, (double) kMaxWidth / image.getWidth());
            w = (int) (image.getWidth()  * scale);
            h = (int) (image.getHeight() * scale);
        }
        setSize (w, h + kFooterH);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0xff111418));

        if (image.isValid())
        {
            auto imgArea = getLocalBounds().withTrimmedBottom (kFooterH()).toFloat();
            g.drawImage (image, imgArea,
                         juce::RectanglePlacement::centred
                       | juce::RectanglePlacement::onlyReduceInSize);
        }

        // Versions-Overlay unten rechts im Bildbereich.
        g.setColour (juce::Colours::white.withAlpha (0.85f));
        g.setFont   (juce::Font (12.0f, juce::Font::bold));
        const juce::String versionLine =
            juce::String (ProjectInfo::projectName) + " "
          + juce::String (ProjectInfo::versionString);
        g.drawText (versionLine,
                    getLocalBounds().withTrimmedBottom (kFooterH()).reduced (12, 8),
                    juce::Justification::bottomRight, false);

        // Footer-Hintergrund.
        auto footer = getLocalBounds().removeFromBottom (kFooterH());
        g.setColour (juce::Colour (0xff1a1f25));
        g.fillRect (footer);
        g.setColour (juce::Colour (0xff2a3038));
        g.fillRect (footer.removeFromTop (1));
    }

    void resized() override
    {
        auto footer = getLocalBounds().removeFromBottom (kFooterH()).reduced (14, 10);
        continueButton.setBounds (footer.removeFromRight (140).reduced (0, 2));
        footer.removeFromRight (14);
        dontShowToggle.setBounds (footer.removeFromLeft (240));
    }

    // Klick auf das Bild dismissiert ebenfalls — Blender-Verhalten.
    void mouseUp (const juce::MouseEvent& e) override
    {
        if (e.eventComponent == this)
            closeDialog();
    }

    // Eintrittspunkt — wird einmal beim Start aufgerufen, wenn die Settings
    // das Anzeigen erlauben.
    static void launch()
    {
        auto* content = new SplashScreenComponent();

        juce::DialogWindow::LaunchOptions o;
        o.content.setOwned (content);
        o.dialogTitle                   = "";
        o.dialogBackgroundColour        = juce::Colour (0xff111418);
        o.escapeKeyTriggersCloseButton  = true;
        o.useNativeTitleBar             = false;
        o.resizable                     = false;
        o.useBottomRightCornerResizer   = false;

        // launchAsync setzt die DialogWindow in den Modal-State -> blockiert
        // Eingaben am Hauptfenster, ohne den Code-Flow zu blocken.
        if (auto* w = o.launchAsync())
            w->setAlwaysOnTop (true);
    }

private:
    static constexpr int kFooterH() { return 56; }

    void closeDialog()
    {
        if (auto* dw = findParentComponentOfClass<juce::DialogWindow>())
            dw->exitModalState (0);
    }

    juce::Image        image;
    juce::TextButton   continueButton;
    juce::ToggleButton dontShowToggle;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SplashScreenComponent)
};
