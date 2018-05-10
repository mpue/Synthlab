/*
  ==============================================================================

  This is an automatically generated GUI class created by the Projucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Projucer version: 5.2.1

  ------------------------------------------------------------------------------

  The Projucer is part of the JUCE library.
  Copyright (c) 2017 - ROLI Ltd.

  ==============================================================================
*/

#pragma once

//[Headers]     -- You can add your own extra header files here --
#include "../JuceLibraryCode/JuceHeader.h"
#include "CustomLookAndFeel.h"
#include "Mixer.h"
#include "VolumeAdjustable.h"
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
                            public Button::Listener
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

    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;

    virtual void changeListenerCallback(juce::ChangeBroadcaster * source) override;
    void setName(juce::String name);
    void setMagnitude(int channel, float magnitude);
    void setMute(bool mute);
    void setSolo(bool solo);
    bool getMute();
    bool getSolo();
    double getVolume();
    void setVolume(float volume);
    double getPan();
    int getIndex();
    void setIndex(int index);
    Mixer::Channel::Type getChannelType();
    void setChannelType(Mixer::Channel::Type type);
    void setChannel(Mixer::Channel* channel);
    void setModule(VolumeAdjustable* v);

    //[/UserMethods]

    void paint (Graphics& g) override;
    void resized() override;
    void sliderValueChanged (Slider* sliderThatWasMoved) override;
    void buttonClicked (Button* buttonThatWasClicked) override;

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

    int index = 0;
    Mixer::Channel* channel;
    VolumeAdjustable* module;
    
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
