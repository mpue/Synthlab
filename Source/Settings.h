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
    }

    void save() const
    {
        juce::XmlElement xml("SynthlabSettings");
        xml.setAttribute("newTrackOutputType",
                         newTrackOutputType == NewTrackOutputType::AuxOut ? "AuxOut" : "AudioOut");
        xml.writeTo(getSettingsFile());
    }

    NewTrackOutputType newTrackOutputType { NewTrackOutputType::AudioOut };
};
