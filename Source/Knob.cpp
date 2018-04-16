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
   
    setSize(80,80);
    nameLabel->setJustificationType (Justification::left);
    nameLabel->setTopLeftPosition(10,58);
    
    
    slider = new Slider ("knob");
    slider->setSize(70,70);
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
    stepsize = 1.0;
    minimum = 0;
    maximum = 127;
}

Knob::~Knob()
{
    delete slider;
}

void Knob::configurePins(){
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
    
    if (isController) {
        value = map(value, 0, 127, minimum, maximum);
    }
    
    this->value = value;
    this->pins.at(0)->setValue(value);
    
    std::function<void(void)> changeLambda =
    [=]() {
        this->nameLabel->setText(String(value),juce::NotificationType::dontSendNotification);
        this->slider->setValue(value, juce::NotificationType::dontSendNotification);
    };
    juce::MessageManager::callAsync(changeLambda);
    

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

void Knob::setIsMidicontroller(bool isController) {
    this->isController = isController;
}

bool Knob::isMidiController() {
    return isController;
}

void Knob::setController(int num) {
    this->controllerNum = num;
}

int Knob::getController() {
    return controllerNum;
}

void Knob::setLearning(bool learning) {
    this->learning = learning;
}

bool Knob::isLearning(){
    return learning;
}

void Knob::eventReceived(Event *e) {
    

}
