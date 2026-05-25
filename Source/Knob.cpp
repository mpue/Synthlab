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
    setSize(90, 90);
    nameLabel->setJustificationType(Justification::left);
    nameLabel->setTopLeftPosition(30, 2);
    nameLabel->setFont(Font(9));

    valueLabel = new Label();
    valueLabel->setJustificationType(Justification::left);
    valueLabel->setTopLeftPosition(2, 70);

    slider = new Slider("knob");
    slider->setSize(70, 70);
    slider->setRange(0, 127, 1);
    slider->setSliderStyle(Slider::RotaryVerticalDrag);
    slider->setTextBoxStyle(Slider::NoTextBox, false, 80, 20);
    slider->setColour(Slider::rotarySliderOutlineColourId, juce::Colours::darkgrey);
    slider->setColour(Slider::thumbColourId, juce::Colours::orange);
    slider->addListener(this);
    slider->addMouseListener(this,true);

    slider->setTopLeftPosition(getWidth() / 2 - slider->getWidth() / 2, getHeight() / 2 - slider->getHeight() / 2);

    addAndMakeVisible(slider);
    addAndMakeVisible(valueLabel);
    valueLabel->toFront(false);

    setInterceptsMouseClicks(true, true);

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
    isControllerValueListener = new IsControllerValueListener(*isControllerValue, this);
    controllerValueListener = new ControllerValueListener(*controllerValue, this);
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

    delete valueListener;
    delete minValueListener;
    delete maxValueListener;
    delete stepsizeValueListener;
    delete isControllerValueListener;
    delete controllerValueListener;

    delete valueLabel;
}

void Knob::configurePins() {
    Pin* p1 = new Pin(Pin::Type::VALUE);
    p1->direction = Pin::Direction::OUT;
    p1->setName("V");
    addPin(Pin::Direction::OUT, p1);

}

void Knob::paint(juce::Graphics& g) {
    // Module::paint(g);    for (int i = 0; i < pins.size();i++) {

    if (currentLayer == Layer::ALL) {
        Module::paint(g);
        if (pins.at(0)->isSelected()) {
            g.setColour(juce::Colours::white);
        }
        else {

            if (pins.at(0)->getType() == Pin::Type::AUDIO) {
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
            g.drawEllipse(pins.at(0)->x, pins.at(0)->y, 10, 10, 1);
            g.fillEllipse(pins.at(0)->x + 2, pins.at(0)->y + 2, 6, 6);
            g.drawText(pins.at(0)->getName(), pins.at(0)->x + 12, pins.at(0)->y, 10, 10, juce::Justification::left);
        }
        else {
            // g.fillRect(pins.at(i)->x + 5, pins.at(i)->y, 5, 10);
            g.drawEllipse(pins.at(0)->x, pins.at(0)->y, 10, 10, 1);
            g.fillEllipse(pins.at(0)->x + 2, pins.at(0)->y + 2, 6, 6);
            g.drawText(pins.at(0)->getName(), pins.at(0)->x - 25, pins.at(0)->y, 20, 10, juce::Justification::right);
        }

        if (pins.at(0)->getConnections().size() >= 1 && pins.at(0)->getConnections().at(0) != nullptr) {
            g.drawText(pins.at(0)->getConnections().at(0)->getName(), 5, 5, 10, 10, juce::Justification::left);
        }

    }

    Component::paint(g);
    if (selected) {
        g.setColour(Colours::orange);
        g.drawRect(0, 0, getWidth(), getHeight());
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
        setSize(90, 30);
        slider->setSize(26, 26);
        slider->setTopLeftPosition(2, 2);
        nameLabel->setTopLeftPosition(30, 2);
        valueLabel->setVisible(false);
        setInterceptsMouseClicks(false, false);
    }
    else {
        // GUI Mode
        setSize(90, 90);
        slider->setSize(70, 70);
        slider->setTopLeftPosition(10, 18);
        nameLabel->setFont(Font(9));
        nameLabel->setBounds(2, 0, 86, 14);
        valueLabel->setVisible(true);
        setInterceptsMouseClicks(false, false);
    }
}

void Knob::setValue(float value) {

    if (isController) {
        value = map(value, 0, 127, minimum, maximum);
    }

    this->value = value;
    this->pins.at(0)->setValue(value);

    // callAsync queues the lambda for a later AppKit dispatch tick. If the
    // Knob (or any of its parents — e.g. via project.clear -> cleanUp ->
    // module destruction) gets deleted between the queueing and the dispatch,
    // a [=]/this-capturing lambda dereferences freed memory. The existing
    // getParentComponent()!=NULL guard does NOT help: it reads the parent
    // member out of *this*, which is itself already freed (caught by ASan
    // in Component::getParentComponent). Component::SafePointer is the
    // standard JUCE remedy — it piggybacks on Component's built-in
    // WeakReference and resolves to nullptr if the component is gone by the
    // time the message fires.
    juce::Component::SafePointer<Knob> safeThis(this);
    juce::MessageManager::callAsync([safeThis, value]() {
        if (auto* self = safeThis.getComponent()) {
            if (self->getParentComponent() != nullptr) {
                if (self->valueLabel != nullptr)
                    self->valueLabel->setText(String(value), juce::NotificationType::dontSendNotification);
                if (self->slider != nullptr)
                    self->slider->setValue(value, juce::NotificationType::dontSendNotification);
            }
        }
    });
}

void Knob::sliderValueChanged(juce::Slider* slider) {

    if (slider == this->slider) {
        this->value = slider->getValue();
        this->valueValue->setValue(this->value);
        // this->nameLabel->setText(String(value),juce::NotificationType::dontSendNotification);
        this->pins.at(0)->setValue(value);
        repaint();
    }
    else {
        setValue(slider->getValue());
    }

}

void Knob::setStepSize(float stepsize) {
    this->stepsize = stepsize;
    this->slider->setRange(minimum, maximum, stepsize);
    this->stepsizeValue->setValue(stepsize);
}

float Knob::getStepsize() {
    return stepsize;
}

void Knob::setMinimum(float min) {
    this->minimum = min;
    this->minValue->setValue(min);
    this->slider->setRange(minimum, maximum, stepsize);
}

float Knob::getMinimum() {
    return minimum;
}

void Knob::setName(juce::String name) {
    Module::setName(name);
}

void Knob::setMaximum(float max) {
    this->maximum = max;
    this->maxValue->setValue(max);
    this->slider->setRange(minimum, maximum, stepsize);
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
    this->controllerValue->setValue(num);
}

int Knob::getController() {
    return controllerNum;
}

void Knob::setLearning(bool learning) {
    this->learning = learning;
}

bool Knob::isLearning() {
    return learning;
}

void Knob::eventReceived(Event* e) {
}

void Knob::setSelected(bool selected) {
    Module::setSelected(selected);
    //slider->setWantsKeyboardFocus(true);
    // slider->grabKeyboardFocus();
}

juce::Array<PropertyComponent*>& Knob::getProperties() {

    // Start with the base Module properties (Name, Display at root)
    Module::getProperties();

    valueProp = new SliderPropertyComponent(*valueValue, "Value", -65535, 65535, 0.01, 1.0, true);
    minValueProp = new SliderPropertyComponent(*minValue, "Minimum", -10000, 15000, 0.01, 1.0, true);
    maxValueProp = new SliderPropertyComponent(*maxValue, "Maximum", -10000, 15000, 0.01, 1.0, true);
    stepsizeValueProp = new SliderPropertyComponent(*stepsizeValue, "Stepsize", 0.001, 10000, 0.01, 1.0, true);
    isControllerValueProp = new BooleanPropertyComponent(*isControllerValue, "isController", "Is controller");
    controllerValueProp = new SliderPropertyComponent(*controllerValue, "Controller number", 0, 127, 1, 1.0, true);

    properties.add(valueProp);
    properties.add(minValueProp);
    properties.add(maxValueProp);
    properties.add(stepsizeValueProp);
    properties.add(isControllerValueProp);
    properties.add(controllerValueProp);

    return properties;
}