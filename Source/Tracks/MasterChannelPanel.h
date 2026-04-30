/*
  ==============================================================================

  This is an automatically generated GUI class created by the Projucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Projucer version: 4.3.0

  ------------------------------------------------------------------------------

  The Projucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright (c) 2015 - ROLI Ltd.

  ==============================================================================
*/

#ifndef __JUCE_HEADER_F8A62509C3ECE492__
#define __JUCE_HEADER_F8A62509C3ECE492__

//[Headers]     -- You can add your own extra header files here --
#include "../JuceLibraryCode/JuceHeader.h"
#include "../CustomLookAndFeel.h"
#include "Track.h"
#include "../Mixer.h"
#include "DragConstrainer.h"
#include "../VolumeAdjustable.h"

// Forward declarations to avoid circular includes
class SynthEditor;
class PluginModule;
class Module;
class Pin;
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Projucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class MasterChannelPanel  : public Component,
                            public ChangeListener,
                            public ChangeBroadcaster,
                            public Slider::Listener,
                            public Button::Listener,
                            public Label::Listener,
                            public juce::Timer
{
public:
    //==============================================================================
    MasterChannelPanel ();
    ~MasterChannelPanel();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.

    enum MeterMode {
        RMS,
        MAGNITUDE
    };

    void mouseDown (const MouseEvent& e) override;
    void mouseDrag (const MouseEvent& e) override;

    virtual void changeListenerCallback(ChangeBroadcaster * source) override;
    void setName(String name);
    void setMagnitude(int channel, float magnitude);
    void updateMeter();
    void setMute(bool mute);
    void setSolo(bool solo);
    bool getMute();
    bool getSolo();
    double getVolume();
    double getPan();
    void setVolume(float volume);
    void setTrack(Track* t);
    int getIndex();
    void setIndex(int index);
    Mixer::Channel::Type getChannelType();
    void setChannelType(Mixer::Channel::Type type);
    void setChannel(Mixer::Channel* channel);
    void setModule(VolumeAdjustable* v);
    void setSynthEditor(SynthEditor* editor);
    void reconstructFXChainFromGraph();
    // Reicht den Transport-State an die FX-Plugins durch — Master-FX-Plugins
    // hängen nicht im Modul-Graph und bekommen sonst keinen Playhead-Update.
    void processMasterFX(juce::AudioBuffer<float>& buf,
                         juce::int64 samplePosition,
                         bool isPlaying,
                         bool isRecording);
    void setFXNonRealtime(bool nonRealtime);

    // Persistence for the master FX chain. Master-channel FX modules live
    // outside the module graph (see insertFX), so the regular saveStructure
    // path doesn't see them — these helpers serialize plugin name, program
    // and state for each occupied slot.
    juce::ValueTree getFXChainState() const;
    void applyFXChainState(const juce::ValueTree& state);
    //[/UserMethods]

    void paint (Graphics& g) override;
    void resized() override;
    void sliderValueChanged (Slider* sliderThatWasMoved) override;
    void buttonClicked (Button* buttonThatWasClicked) override;
    void labelTextChanged (Label* labelThatHasChanged) override;
    void timerCallback() override;

    // Binary resources:
    static const char* peaks_png;
    static const int peaks_pngSize;
    static const char* round_button_png;
    static const int round_button_pngSize;
    static const char* round_button_pushed_png;
    static const int round_button_pushed_pngSize;


private:
    //[UserVariables]   -- You can add your own custom variables in this section.

    // ComponentDragger dragger;
	// DragConstrainer constrainer;
    MeterMode mode;
    bool link;
    bool mute = false;
    bool solo = false;

    double volume = 1.0f;
    double pan = 0.0f;

    // VU meter animation: target = raw peak, smoothed = animated display value
    float targetL   = 0.0f;
    float targetR   = 0.0f;
    float smoothedL = 0.0f;
    float smoothedR = 0.0f;

    // Peak-hold indicator: holds for ~2 s then falls smoothly
    float peakL       = 0.0f;
    float peakR       = 0.0f;
    int   peakHoldL   = 0;     // countdown in timer ticks (60 Hz)
    int   peakHoldR   = 0;

    Track* track = nullptr;

    CustomLookAndFeel* clf;

    int index = 0;
    Mixer::Channel* channel;
    VolumeAdjustable* module;

    // --- FX chain ---
    static constexpr int NUM_FX = 4;
    SynthEditor*  synthEditor = nullptr;
    PluginModule* fxChain[NUM_FX]      = {};
    juce::TextButton* fxOpenButtons[NUM_FX] = {};

    // Source modules/pins feeding into the AudioOut before any FX
    Module* chainSourceModL = nullptr;
    Module* chainSourceModR = nullptr;
    Pin*    chainSourcePinL = nullptr;
    Pin*    chainSourcePinR = nullptr;

    int  getSlotAtPoint(juce::Point<int> pos) const;
    void onFXSlotClicked(int slotIndex);
    void insertFX(int slotIndex, juce::String pluginName);
    void removeFX(int slotIndex);
    void rewireChain();
    void updateFXButtons();
    //[/UserVariables]

    //==============================================================================
    ScopedPointer<Slider> vuSliderL;
    ScopedPointer<Slider> channelVolume;
    ScopedPointer<Slider> panSlider;
    ScopedPointer<Label> nameLabel;
    ScopedPointer<ImageButton> muteButton;
    ScopedPointer<ImageButton> soloButton;
    ScopedPointer<Label> M;
    ScopedPointer<Label> S;
    ScopedPointer<Slider> vuSliderR;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MasterChannelPanel)
};

//[EndFile] You can add extra defines here...
//[/EndFile]

#endif   // __JUCE_HEADER_F8A62509C3ECE492__
