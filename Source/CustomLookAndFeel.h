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

class CustomLookAndFeel : public LookAndFeel_V3 {
    
public:
    
    CustomLookAndFeel() {
        gradientVertical = ColourGradient(Colours::red, 0, 0, Colours::chartreuse, 5, 128, false);
        gradientVertical.addColour(0.2 , Colours::orange);
        
        gradientHorizontal = ColourGradient(Colours::chartreuse, 0, 0, Colours::red, 128, 5, false);
        gradientHorizontal.addColour(0.8 , Colours::orange);
        
        setColour(Slider::rotarySliderFillColourId, Colours::steelblue.darker());
        setColour(Slider::trackColourId, Colours::transparentBlack);
        setColour(Slider::thumbColourId, Colours::transparentBlack);
        setColour(DialogWindow::backgroundColourId, Colours::darkgrey);
        setColour(Label::textColourId, Colours::white);
        setColour(ComboBox::backgroundColourId, Colours::black);
        setColour(PopupMenu::backgroundColourId, Colours::black);
        setColour(PopupMenu::highlightedBackgroundColourId, Colours::steelblue.brighter());
        setColour(PopupMenu::textColourId, Colours::steelblue.brighter());
        setColour(ComboBox::textColourId, Colours::steelblue.brighter());
        setColour(ComboBox::outlineColourId, Colours::steelblue.brighter());
        setColour(ComboBox::arrowColourId, Colours::steelblue.brighter());
        setColour(ComboBox::buttonColourId, Colours::steelblue.brighter());
        setColour(ListBox::backgroundColourId, Colours::black);
        setColour(ListBox::textColourId, Colours::steelblue.brighter());
        setColour(TextButton::buttonColourId, Colours::steelblue.darker());
        setColour(TextButton::buttonOnColourId, Colours::steelblue.darker());
        setColour(TextButton::textColourOnId, Colours::white);
        setColour(TextButton::textColourOffId, Colours::white);
        setColour(TextEditor::outlineColourId, Colours::steelblue.darker());
        setColour(TextEditor::backgroundColourId, Colours::black);
        setColour(TextEditor::highlightColourId, Colours::steelblue.darker());
        setColour(TextEditor::highlightedTextColourId, Colours::black);
        setColour(TextEditor::textColourId, Colours::steelblue.brighter());
        setColour(TextButton::textColourOffId, Colours::steelblue.darker());
        setColour(TextButton::textColourOnId, Colours::black);
        setColour(CaretComponent::caretColourId, Colours::steelblue);
        setColour (ScrollBar::thumbColourId,Colours::steelblue.darker());
        setColour (ScrollBar::backgroundColourId,Colour::fromRGBA(0x5a,0x5a,0x5a,0xff));
        setColour (Slider::thumbColourId,Colours::steelblue.darker());
        setColour (Slider::trackColourId,Colour::fromRGBA(0x5a,0x5a,0x5a,0xff));
        
    }
    
    void drawLinearSlider (Graphics&, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           const Slider::SliderStyle, Slider&) override;
    
private:
    ColourGradient gradientVertical;
    ColourGradient gradientHorizontal;
    ComponentAnimator animator;
    
    long triggerTime = 0;
    int maxY = 0xFFFF;
    int maxX = 0;
};



#endif  // CUSTOMLOOKANDFEEL_H_INCLUDED
