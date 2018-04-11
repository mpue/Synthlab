/*
  ==============================================================================

    Constant
    Created: 4 Apr 2018 3:48:23pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "Knob.h"


//==============================================================================
Knob::Knob()
{
   
    setSize(100,100);
    nameLabel->setJustificationType (Justification::left);
    nameLabel->setTopLeftPosition(2,2);

    slider = new Slider ("knob");
    slider->setSize(80,80);
    slider->setRange (0, 127, 1);
    slider->setSliderStyle (Slider::RotaryVerticalDrag);
    slider->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    // slider->setColour (Slider::rotarySliderOutlineColourId, Colour (0x66ffffff));
    slider->addListener (this);
    
    
    slider->setTopLeftPosition(getWidth() / 2 - slider->getWidth() / 2, getHeight() / 2 - slider->getHeight()/2);
    
    
    addAndMakeVisible(slider);
    

    // slider->toFront(true);
    setInterceptsMouseClicks(false, true);
    
    editable = false;
    prefab = true;
    minimum = 0;
    maximum = 127;
}

Knob::~Knob()
{
    addPin(Pin::Direction::OUT);
    pins.at(0)->setName("");
    delete slider;
}

void Knob::configurePins() {
    Pin* p1 = new Pin(Pin::Type::VALUE);
    p1->direction = Pin::Direction::OUT;
    p1->setName("V");
    addPin(Pin::Direction::OUT,p1);

}
/*
void Knob::paint(juce::Graphics &g) {
    // Module::paint(g);
    Component::paint(g);
}
 */

float Knob::getValue() {
    return value;
}

void Knob::setValue(float value) {
    this->value = value;
    this->pins.at(0)->setValue(value);
    this->nameLabel->setText(String(value),juce::NotificationType::dontSendNotification);
}

void Knob::sliderValueChanged(juce::Slider *slider) {
    setValue(slider->getValue());
}

void Knob::setStepSize(float stepsize) {
    this->stepsize = stepsize;
    this->slider->setRange(minimum,  maximum, stepsize);
}

float Knob::getStepsize() {
    return stepsize;
}

void Knob::setMinimum(float min) {
    this->minimum = min;
    this->slider->setRange(minimum,  maximum, stepsize);
}

float Knob::getMinimum() {
    return minimum;
}

void Knob::setMaximum(float max) {
    this->maximum = max;
    this->slider->setRange(minimum,  maximum, stepsize);
}

float Knob::getMaximum() {
    return maximum;
}
