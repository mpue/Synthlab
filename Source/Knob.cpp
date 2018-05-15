/*
  ==============================================================================

    Constant
    Created: 4 Apr 2018 3:48:23pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "Knob.h"

using juce::Justification;
using juce::Slider;
using juce::Value;
using juce::String;
using juce::PropertyComponent;
using juce::SliderPropertyComponent;
using juce::BooleanPropertyComponent;

//==============================================================================
Knob::Knob()
{
    setSize(90,90);
    nameLabel->setJustificationType (Justification::left);
    nameLabel->setTopLeftPosition(30 ,2);
    nameLabel->setFont(Font(9));
    
    valueLabel = new Label();
    valueLabel->setJustificationType (Justification::left);
    valueLabel->setTopLeftPosition(2,70);
    
    slider = new Slider ("knob");
    slider->setSize(70,70);
    slider->setRange (0, 127, 1);
    slider->setSliderStyle (Slider::RotaryVerticalDrag);
    slider->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    slider->setColour (Slider::rotarySliderOutlineColourId, juce::Colours::darkgrey);
    slider->setColour(Slider::thumbColourId, juce::Colours::orange);
    slider->addListener (this);
    
    slider->setTopLeftPosition(getWidth() / 2 - slider->getWidth() / 2, getHeight() / 2 - slider->getHeight()/2);
    
    addAndMakeVisible(slider);
    addAndMakeVisible(valueLabel);
    valueLabel->toFront(false);
    
    setInterceptsMouseClicks(false,true);
    
    createProperties();
    
    editable = false;
    prefab = true;
    setStepSize(1);
    setMinimum(0);
    setMaximum(127);
    
    setCurrentLayer(Layer::ALL);
    
    slider->setEnabled(false);
    
}

void Knob::createProperties() {
    nameValue = new Value(); 
    valueValue = new Value();
    minValue = new Value();
    maxValue = new Value();
    stepsizeValue = new Value();
    isControllerValue = new Value();
    controllerValue = new Value();
    valueListener = new ValueListener(*valueValue, this);
    minValueListener = new MinValueListener(*minValue, this);
    maxValueListener = new MaxValueListener(*maxValue, this);
    stepsizeValueListener = new StepsizeValueListener(*stepsizeValue, this);
    isControllerValueListener = new IsControllerValueListener(*isControllerValue,this);
    controllerValueListener = new ControllerValueListener(*controllerValue,this);
    nameListener = new NameListener(*nameValue, this);
}


Knob::~Knob()
{
    delete slider;

    delete valueValue;
    delete stepsizeValue;
    delete isControllerValue;
    delete controllerValue;
    delete minValue;
    delete maxValue;
    delete nameValue;
    
    delete valueListener;
    delete minValueListener;
    delete maxValueListener;
    delete stepsizeValueListener;
    delete isControllerValueListener;
    delete controllerValueListener;
    delete nameListener;
    
    delete valueLabel;
}

void Knob::configurePins(){
    Pin* p1 = new Pin(Pin::Type::VALUE);
    p1->direction = Pin::Direction::OUT;
    p1->setName("V");
    addPin(Pin::Direction::OUT,p1);

}

void Knob::paint(juce::Graphics &g) {
    // Module::paint(g);    for (int i = 0; i < pins.size();i++) {
    
    if (currentLayer == Layer::ALL) {
        Module::paint(g);
        if (pins.at(0)->isSelected()) {
            g.setColour(juce::Colours::white);
        }
        else {
            
            if(pins.at(0)->getType() == Pin::Type::AUDIO) {
                g.setColour(juce::Colours::orange);
            }
            else if (pins.at(0)->getType() == Pin::Type::VALUE) {
                g.setColour(juce::Colours::yellow);
            }
            else {
                g.setColour(juce::Colours::lightblue);
            }
            
        }
        
        g.setFont(8);
        
        if (pins.at(0)->direction == Pin::Direction::IN) {
            //g.fillRect(pins.at(i)->x,pins.at(i)->y, 5, 10);
            g.drawEllipse(pins.at(0)->x,pins.at(0)->y, 10, 10,1);
            g.fillEllipse(pins.at(0)->x+2,pins.at(0)->y+2, 6, 6);
            g.drawText(pins.at(0)->getName() , pins.at(0)->x + 12,pins.at(0)->y, 10, 10, juce::Justification::left);
        }
        else {
            // g.fillRect(pins.at(i)->x + 5, pins.at(i)->y, 5, 10);
            g.drawEllipse(pins.at(0)->x, pins.at(0)->y, 10, 10,1);
            g.fillEllipse(pins.at(0)->x+2,pins.at(0)->y +2, 6, 6);
            g.drawText(pins.at(0)->getName() , pins.at(0)->x - 25,pins.at(0)->y, 20, 10, juce::Justification::right);
        }
        
        if (pins.at(0)->getConnections().size() >= 1 &&  pins.at(0)->getConnections().at(0) != nullptr) {
            g.drawText(pins.at(0)->getConnections().at(0)->getName(),5,5, 10, 10, juce::Justification::left);
        }
    
    }

    Component::paint(g);
    if (selected) {
        g.setColour(Colours::orange);
        g.drawRect(0,0,getWidth(),getHeight());
    }
    
    g.setColour(Colours::white);
    g.drawText(String(value), 2, getHeight() - 10, 50, 10, juce::Justification::right);
    

}


float Knob::getValue() {
    return value;
}

void Knob::setCurrentLayer(Module::Layer layer) {
    Module::setCurrentLayer(layer);
    
    if (currentLayer == Module::Layer::ALL) {
        setSize(90,30);
        slider->setSize(26,26);
        slider->setTopLeftPosition(2,2);
        nameLabel->setTopLeftPosition(30 ,2);
        valueLabel->setVisible(false);
        setInterceptsMouseClicks(false,false);
    }
    else {
        // GUI Mode
        setSize(90,90);
        slider->setSize(70,70);
        slider->setTopLeftPosition(10,10);
        nameLabel->setFont(Font(9));
        nameLabel->setTopLeftPosition(2 ,2);
        valueLabel->setVisible(true);
        setInterceptsMouseClicks(false,false);
    }
}

void Knob::setValue(float value) {
    
    if (isController) {
        value = map(value, 0, 127, minimum, maximum);
    }
    
    this->value = value;
    this->pins.at(0)->setValue(value);
    
    std::function<void(void)> changeLambda =
    [=]() {
        
        if (getParentComponent() != NULL) {
            this->valueLabel->setText(String(value),juce::NotificationType::dontSendNotification);
            this->slider->setValue(value, juce::NotificationType::dontSendNotification);
        }
    };
    juce::MessageManager::callAsync(changeLambda);
    

}

void Knob::sliderValueChanged(juce::Slider *slider) {
    this->value = slider->getValue();
    this->valueValue->setValue(this->value);
    // this->nameLabel->setText(String(value),juce::NotificationType::dontSendNotification);
    this->pins.at(0)->setValue(value);
    repaint();
}

void Knob::setStepSize(float stepsize) {
    this->stepsize = stepsize;
    this->slider->setRange(minimum,  maximum, stepsize);
    this->stepsizeValue->setValue(stepsize);
}

float Knob::getStepsize() {
    return stepsize;
}

void Knob::setMinimum(float min) {
    this->minimum = min;
    this->minValue->setValue(min);
    this->slider->setRange(minimum,  maximum, stepsize);
}

float Knob::getMinimum() {
    return minimum;
}

void Knob::setName(juce::String name) {
    Module::setName(name);
    nameValue->setValue(name);
}

void Knob::setMaximum(float max) {
    this->maximum = max;
    this->maxValue->setValue(max);
    this->slider->setRange(minimum,  maximum, stepsize);
}

float Knob::getMaximum() {
    return maximum;
}

void Knob::setIsMidicontroller(bool isController) {
    this->isController = isController;
    this->isControllerValue->setValue(isController);
}

bool Knob::isMidiController() {
    return isController;
}

void Knob::setController(int num) {
    this->controllerNum = num;
    this->controllerValue->setValue(controllerValue);
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

void Knob::setSelected(bool selected) {
    Module::setSelected(selected);
    //slider->setWantsKeyboardFocus(true);
    // slider->grabKeyboardFocus();
}

juce::Array<PropertyComponent*>& Knob::getProperties() {
    
    properties = juce::Array<PropertyComponent*>();
    
    valueProp = new SliderPropertyComponent(*valueValue,"Value",-65535,65535,0.01,1.0,true);
    minValueProp = new SliderPropertyComponent(*minValue,"Minimum",-10000,15000,0.01,1.0,true);
    maxValueProp = new SliderPropertyComponent(*maxValue,"Maximum",-10000,15000,0.01,1.0,true);
    stepsizeValueProp = new SliderPropertyComponent(*stepsizeValue,"Stepsize",0.001,10000,0.01,1.0,true);
    isControllerValueProp = new BooleanPropertyComponent(*isControllerValue,"isController","Is controller");
    controllerValueProp = new SliderPropertyComponent(*controllerValue,"Controller number",0,127,1,1.0,true);
    nameValueProp = new TextPropertyComponent(*nameValue,"Name",16, false,true);
    
    properties.add(nameValueProp);
    properties.add(valueProp);
    properties.add(minValueProp);
    properties.add(maxValueProp);
    properties.add(stepsizeValueProp);
    properties.add(isControllerValueProp);
    properties.add(controllerValueProp);
    
    return properties;
}
