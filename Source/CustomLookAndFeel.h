/*
  ==============================================================================

    CustomLookAndFeel.h
    Created: 25 Nov 2016 5:14:44pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#ifndef CUSTOMLOOKANDFEEL_H_INCLUDED
#define CUSTOMLOOKANDFEEL_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"

class CustomLookAndFeel : public juce::LookAndFeel_V3 {
    
public:
    
    CustomLookAndFeel() {
        gradientVertical = juce::ColourGradient(juce::Colours::red, 0, 0, juce::Colours::chartreuse, 5, 128, false);
        gradientVertical.addColour(0.2 , juce::Colours::orange);
        
        gradientHorizontal = juce::ColourGradient(juce::Colours::chartreuse, 0, 0, juce::Colours::red, 128, 5, false);
        gradientHorizontal.addColour(0.8 , juce::Colours::orange);
        
        setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::steelblue.darker());
        setColour(juce::Slider::trackColourId, juce::Colours::transparentBlack);
        setColour(juce::Slider::thumbColourId, juce::Colours::transparentBlack);
        setColour(juce::DialogWindow::backgroundColourId, juce::Colours::darkgrey);
        setColour(juce::Label::textColourId, juce::Colours::white);
        setColour(juce::ComboBox::backgroundColourId, juce::Colours::black);
        setColour(juce::PopupMenu::backgroundColourId, juce::Colours::black);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colours::steelblue.brighter());
        setColour(juce::PopupMenu::textColourId, juce::Colours::steelblue.brighter());
        setColour(juce::ComboBox::textColourId, juce::Colours::steelblue.brighter());
        setColour(juce::ComboBox::outlineColourId, juce::Colours::steelblue.brighter());
        setColour(juce::ComboBox::arrowColourId, juce::Colours::steelblue.brighter());
        setColour(juce::ComboBox::buttonColourId, juce::Colours::steelblue.brighter());
        setColour(juce::ListBox::backgroundColourId, juce::Colours::black);
        setColour(juce::ListBox::textColourId, juce::Colours::steelblue.brighter());
        setColour(juce::TextButton::buttonColourId, juce::Colours::steelblue.darker());
        setColour(juce::TextButton::buttonOnColourId, juce::Colours::steelblue.darker());
        setColour(juce::TextButton::textColourOnId, juce::Colours::white);
        setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        setColour(juce::TextEditor::outlineColourId, juce::Colours::steelblue.darker());
        setColour(juce::TextEditor::backgroundColourId, juce::Colours::black);
        setColour(juce::TextEditor::highlightColourId, juce::Colours::steelblue.darker());
        setColour(juce::TextEditor::highlightedTextColourId, juce::Colours::black);
        setColour(juce::TextEditor::textColourId, juce::Colours::steelblue.brighter());
        setColour(juce::TextButton::textColourOffId, juce::Colours::steelblue.darker());
        setColour(juce::TextButton::textColourOnId, juce::Colours::black);
        setColour(juce::CaretComponent::caretColourId, juce::Colours::steelblue);
        setColour (juce::ScrollBar::thumbColourId,juce::Colours::steelblue.darker());
        setColour (juce::ScrollBar::backgroundColourId, juce::Colour::fromRGBA(0x5a,0x5a,0x5a,0xff));
        setColour (juce::Slider::thumbColourId,juce::Colours::steelblue.darker());
        setColour (juce::Slider::trackColourId, juce::Colour::fromRGBA(0x5a,0x5a,0x5a,0xff));
        
    }
    
    void drawLinearSlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           const juce::Slider::SliderStyle, juce::Slider&) override;
    
private:
    juce::ColourGradient gradientVertical;
    juce::ColourGradient gradientHorizontal;
    juce::ComponentAnimator animator;
    
    long triggerTime = 0;
    int maxY = 0xFFFF;
    int maxX = 0;
};



#endif  // CUSTOMLOOKANDFEEL_H_INCLUDED
