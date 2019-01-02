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

#ifndef __JUCE_HEADER_36DD6F41998FBECE__
#define __JUCE_HEADER_36DD6F41998FBECE__

//[Headers]     -- You can add your own extra header files here --
#include "../JuceLibraryCode/JuceHeader.h"
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Projucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class ImageToggleButton  : public Component,
public Button::Listener
{
public:
    //==============================================================================
    ImageToggleButton ();
    ~ImageToggleButton();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
    void setToggleState(bool toggle, juce::NotificationType notification);
    void setText(String text);
    
    void setImages (bool resizeButtonNowToFitThisImage,
                    bool rescaleImagesWhenButtonSizeChanges,
                    bool preserveImageProportions,
                    const Image& normalImage,
                    float imageOpacityWhenNormal,
                    Colour overlayColourWhenNormal,
                    const Image& overImage,
                    float imageOpacityWhenOver,
                    Colour overlayColourWhenOver,
                    const Image& downImage,
                    float imageOpacityWhenDown,
                    Colour overlayColourWhenDown,
                    float hitTestAlphaThreshold = 0.0f);
    
    Button* getButton();
    
    //[/UserMethods]

    void paint (Graphics& g) override;
    void resized() override;
    void buttonClicked (Button* buttonThatWasClicked) override;

    // Binary resources:
    static const char* round_button_png;
    static const int round_button_pngSize;
    static const char* round_button_pushed_png;
    static const int round_button_pushed_pngSize;


private:
    //[UserVariables]   -- You can add your own custom variables in this section.

    //[/UserVariables]

    //==============================================================================
    ScopedPointer<ImageButton> toggleButton;
    ScopedPointer<Label> label;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ImageToggleButton)
};

//[EndFile] You can add extra defines here...
//[/EndFile]

#endif   // __JUCE_HEADER_36DD6F41998FBECE__
