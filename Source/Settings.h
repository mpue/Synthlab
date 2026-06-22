/*
  ==============================================================================

    Settings.h
    Application-wide preferences (separate from project state and from the
    Audio-Device config in ~/.Synthlab/config.xml). Header-only singleton,
    backed by ~/.Synthlab/settings.xml.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

class Settings
{
public:
    enum class NewTrackOutputType { AudioOut, AuxOut };

    static Settings* getInstance()
    {
        static Settings instance;
        return &instance;
    }

    NewTrackOutputType getNewTrackOutputType() const { return newTrackOutputType; }

    void setNewTrackOutputType(NewTrackOutputType t)
    {
        if (newTrackOutputType == t) return;
        newTrackOutputType = t;
        save();
    }

    bool getOpenPluginWindowAutomatically() const { return openPluginWindowAutomatically; }

    void setOpenPluginWindowAutomatically(bool b)
    {
        if (openPluginWindowAutomatically == b) return;
        openPluginWindowAutomatically = b;
        save();
    }

    // Plugin-Editoren als dockbare Panels statt freischwebender Fenster zeigen.
    // Default aus -> klassisches DocumentWindow. Greift beim naechsten Oeffnen
    // eines Plugin-Fensters; bereits offene Fenster bleiben wie sie sind.
    bool getDockPluginWindows() const { return dockPluginWindows; }

    void setDockPluginWindows(bool b)
    {
        if (dockPluginWindows == b) return;
        dockPluginWindows = b;
        save();
    }

    // signalsmith-stretch Tuning. Beide Werte sind Anteile der Sample-Rate
    // (Block bzw. Intervall). Default-Preset der Lib waere 0.12 / 0.03;
    // wir starten etwas weicher (0.18 / 0.02) — der User kann nachregeln.
    double getStretchBlockPercent() const    { return stretchBlockPercent; }
    double getStretchIntervalPercent() const { return stretchIntervalPercent; }
    bool   getStretchFormantPreservation() const { return stretchFormantPreservation; }

    void setStretchBlockPercent(double v)
    {
        v = juce::jlimit(0.05, 0.30, v);
        if (std::abs(stretchBlockPercent - v) < 1e-6) return;
        stretchBlockPercent = v;
        save();
    }

    void setStretchIntervalPercent(double v)
    {
        v = juce::jlimit(0.005, 0.08, v);
        if (std::abs(stretchIntervalPercent - v) < 1e-6) return;
        stretchIntervalPercent = v;
        save();
    }

    void setStretchFormantPreservation(bool b)
    {
        if (stretchFormantPreservation == b) return;
        stretchFormantPreservation = b;
        save();
    }

    // Splash beim Start. Default true — Toggle im Audio-Settings-Dialog
    // und im Splash selbst ("Don't show on startup").
    bool getShowSplashOnStartup() const { return showSplashOnStartup; }

    void setShowSplashOnStartup(bool b)
    {
        if (showSplashOnStartup == b) return;
        showSplashOnStartup = b;
        save();
    }

    // Auto Track Arming. Wenn aktiv, schaltet ein Klick auf einen Track diesen
    // record-scharf und disarmt alle anderen. Default aus.
    bool getAutoTrackArming() const { return autoTrackArming; }

    void setAutoTrackArming(bool b)
    {
        if (autoTrackArming == b) return;
        autoTrackArming = b;
        save();
    }

    // Basis-URL des Lizenzservers (ohne abschliessendes /activate). Default ist
    // der Produktivserver; zur Laufzeit umstellbar (z.B. http://localhost:4000/
    // api/activation fuer lokale Tests). Leerer Wert -> LicenseManager nutzt
    // seinen eingebauten Default.
    juce::String getLicenseServerUrl() const { return licenseServerUrl; }

    void setLicenseServerUrl(const juce::String& u)
    {
        const juce::String v = u.trim();
        if (licenseServerUrl == v) return;
        licenseServerUrl = v;
        save();
    }

private:
    Settings() { load(); }

    static juce::File getSettingsFile()
    {
        const juce::String home = juce::File::getSpecialLocation(juce::File::userHomeDirectory).getFullPathName();
        juce::File appDir(home + "/.Synthlab");
        if (! appDir.exists())
            appDir.createDirectory();
        return juce::File(home + "/.Synthlab/settings.xml");
    }

    void load()
    {
        const juce::File f = getSettingsFile();
        if (! f.existsAsFile()) return;

        std::unique_ptr<juce::XmlElement> xml(juce::XmlDocument::parse(f));
        if (xml == nullptr) return;

        const juce::String t = xml->getStringAttribute("newTrackOutputType", "AudioOut");
        newTrackOutputType = (t == "AuxOut") ? NewTrackOutputType::AuxOut
                                             : NewTrackOutputType::AudioOut;

        openPluginWindowAutomatically =
            xml->getBoolAttribute("openPluginWindowAutomatically", false);

        dockPluginWindows =
            xml->getBoolAttribute("dockPluginWindows", false);

        stretchBlockPercent =
            juce::jlimit(0.05, 0.30, xml->getDoubleAttribute("stretchBlockPercent", 0.18));
        stretchIntervalPercent =
            juce::jlimit(0.005, 0.08, xml->getDoubleAttribute("stretchIntervalPercent", 0.02));
        stretchFormantPreservation =
            xml->getBoolAttribute("stretchFormantPreservation", true);

        showSplashOnStartup =
            xml->getBoolAttribute("showSplashOnStartup", true);

        autoTrackArming =
            xml->getBoolAttribute("autoTrackArming", false);

        licenseServerUrl =
            xml->getStringAttribute("licenseServerUrl", licenseServerUrl);
    }

    void save() const
    {
        juce::XmlElement xml("SynthlabSettings");
        xml.setAttribute("newTrackOutputType",
                         newTrackOutputType == NewTrackOutputType::AuxOut ? "AuxOut" : "AudioOut");
        xml.setAttribute("openPluginWindowAutomatically", openPluginWindowAutomatically);
        xml.setAttribute("dockPluginWindows",             dockPluginWindows);
        xml.setAttribute("stretchBlockPercent",        stretchBlockPercent);
        xml.setAttribute("stretchIntervalPercent",     stretchIntervalPercent);
        xml.setAttribute("stretchFormantPreservation", stretchFormantPreservation);
        xml.setAttribute("showSplashOnStartup",        showSplashOnStartup);
        xml.setAttribute("autoTrackArming",            autoTrackArming);
        xml.setAttribute("licenseServerUrl",           licenseServerUrl);
        xml.writeTo(getSettingsFile());
    }

    NewTrackOutputType newTrackOutputType { NewTrackOutputType::AudioOut };
    bool               openPluginWindowAutomatically { false };
    bool               dockPluginWindows             { false };
    double             stretchBlockPercent        { 0.18 };
    double             stretchIntervalPercent     { 0.02 };
    bool               stretchFormantPreservation { true };
    bool               showSplashOnStartup        { true };
    bool               autoTrackArming            { false };
    juce::String       licenseServerUrl           { "https://shop.pueski.de/api/activation"};
};
