/*
  ==============================================================================

    VolumeAdjustable.h
    Created: 10 May 2018 10:51:15am
    Author:  Matthias Pueski

  ==============================================================================
*/

#pragma once
#include "../JuceLibraryCode/JuceHeader.h"

class VolumeAdjustable : public ChangeBroadcaster {
public:
    
    virtual ~VolumeAdjustable() {
        removeAllChangeListeners();
    };
    
    virtual void setVolume(float volume) {
        this->volume = volume;
        sendChangeMessage();
    }
    
    virtual float getVolume() {
        return volume;
    }

    // User-vergebener Channel-Strip-Name fuer Hardware-Channels (Audio-In/Out,
    // Aux). Wird ueber ModuleUtils::save-/loadStructure als "channelName"-Property
    // auf dem zugehoerigen Modul persistiert; configureAudioModule schreibt
    // ihn nach dem Default-Init auf den MasterChannelPanel und Mixer::Channel
    // zurueck. Fuer Track-gebundene Channels ist die Source-of-truth weiterhin
    // Track::getName(), das Feld bleibt dann leer.
    void setSavedChannelName(const juce::String& n) { savedChannelName = n; }
    const juce::String& getSavedChannelName() const   { return savedChannelName; }

protected:
    // Unity-Default (0 dB). AddModuleAction liest beim Anlegen eines AuxOut/
    // BusOut/AudioIn diesen Wert ueber getVolume() und schreibt ihn auf den
    // frischen Mixer-Kanal — mit dem alten 0-Default landeten neu erzeugte
    // Tracks im Mixer auf "stumm", bis der User den Fader anfasste.
    float volume = 1.0f;
    juce::String savedChannelName;
};
