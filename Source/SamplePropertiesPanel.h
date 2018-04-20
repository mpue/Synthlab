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
#include "SamplerModule.h"
#include "AudioThumbnailComponent.h"
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Projucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class SamplePropertiesPanel  : public Component,
                               public Button::Listener,
                               public Slider::Listener
{
public:
    //==============================================================================
    SamplePropertiesPanel ();
    ~SamplePropertiesPanel();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
    void setModule(SamplerModule* module);
    void setThumbnail(AudioThumbnailComponent* thumbnail);
    void openSample();
    void updateValues();

    //[/UserMethods]

    void paint (Graphics& g) override;
    void resized() override;
    void buttonClicked (Button* buttonThatWasClicked) override;
    void sliderValueChanged (Slider* sliderThatWasMoved) override;



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
    SamplerModule* module;
    AudioThumbnailComponent* thumb;
    //[/UserVariables]

    //==============================================================================
    ScopedPointer<TextButton> loadSampleButton;
    ScopedPointer<Slider> sampleStartSlider;
    ScopedPointer<Label> startLabel;
    ScopedPointer<Label> endLabel;
    ScopedPointer<Slider> sampleEndSlider;
    ScopedPointer<ToggleButton> loopToggleButton;
    ScopedPointer<Slider> pitchSlider;
    ScopedPointer<Label> pitchLabel;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SamplePropertiesPanel)
};

//[EndFile] You can add extra defines here...
//[/EndFile]
