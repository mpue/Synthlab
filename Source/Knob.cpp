/*
  ==============================================================================

    Constant
    Created: 4 Apr 2018 3:48:23pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "Knob.h"
#include "SynthEditor.h"

// Forward-Deklaration für bessere Performance
class SynthEditor;

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
        
    setSize(90,128);
    nameLabel->setJustificationType (Justification::left);
    nameLabel->setTopLeftPosition(30 ,2);
    juce::Font font("Arial", 12.0f, juce::Font::bold);
    nameLabel->setFont(font);
        
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
    
    // Slider-Performance verbessern für schnelle Mausbewegungen
    slider->setMouseDragSensitivity(200);  // Höhere Sensitivität für feinere Kontrolle
    slider->setVelocityBasedMode(true);    // Velocity-basierter Drag-Modus
    slider->setVelocityModeParameters(1.0, 1, 0.09, false); // Optimale Parameter für Rotary-Knobs
    
    slider->setTopLeftPosition(getWidth() / 2 - slider->getWidth() / 2, getHeight() / 2 - slider->getHeight()/2);
    
    addAndMakeVisible(slider);
    addAndMakeVisible(valueLabel);
    valueLabel->toFront(false);
    
    setInterceptsMouseClicks(false,false);
    
    createProperties();
    
    editable = false;
    prefab = true;
    setStepSize(1);
    setMinimum(0);
    setMaximum(127);
    
    setCurrentLayer(Layer::ALL);
    
    // Slider standardmäßig aktiviert lassen - wird durch setLocked() korrekt gesetzt
    
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
    
    // Selection Highlight (immer über allem anderen zeichnen)
    if (isShowingSelectionHighlight && !locked) {
        g.setColour(juce::Colours::lightblue.withAlpha(0.3f));
        
        if (currentLayer == Layer::ALL) {
            // ALL-Modus: Name-Label-Bereich hervorheben
            Rectangle<int> highlightArea(30, 0, 60, 30);
            g.fillRoundedRectangle(highlightArea.toFloat(), 3.0f);
            g.setColour(juce::Colours::lightblue.withAlpha(0.6f));
            g.drawRoundedRectangle(highlightArea.toFloat(), 3.0f, 1.0f);
        } else {
            // GUI-Modus: Name-Label-Bereich hervorheben
            Rectangle<int> highlightArea(0, 0, 90, 12);
            g.fillRoundedRectangle(highlightArea.toFloat(), 3.0f);
            g.setColour(juce::Colours::lightblue.withAlpha(0.6f));
            g.drawRoundedRectangle(highlightArea.toFloat(), 3.0f, 1.0f);
        }
    }
    
    // Standard-Selektion (orange Rahmen)
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
        setInterceptsMouseClicks(true, false); // Modul fängt Mouse-Events ab für intelligente Behandlung
    }
    else {
        // GUI Mode
        setSize(90,90);
        slider->setSize(70,70);
        slider->setTopLeftPosition(10,10);
        nameLabel->setTopLeftPosition(2 ,2);
        valueLabel->setVisible(true);
        setEnabled(true);
        setInterceptsMouseClicks(true, false); // Modul fängt Mouse-Events ab für intelligente Behandlung      
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
    
    if (slider == this->slider) {
        this->value = (float)slider->getValue();
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
    this->controllerValue->setValue(num);
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

void Knob::mouseDown(const MouseEvent& e) {
    isDraggingSlider = false;
    isDraggingModule = false;
    
    if (isInSliderArea(e.getPosition())) {
        // Klick ist im Slider-Bereich - weiterleiten an Slider (immer erlaubt)
        isDraggingSlider = true;
        MouseEvent relativeEvent = e.getEventRelativeTo(slider);
        slider->mouseDown(relativeEvent);
        return;
    }
    
    if (isInSelectionArea(e.getPosition()) && !locked) {
        // Klick im Selektions-Bereich - für Selektion/Verschiebung
        isDraggingModule = true;
        handleModuleSelection();
    }
}

void Knob::handleModuleSelection() {
    // Direkte Selektion-Behandlung
    if (auto* synthEditor = dynamic_cast<SynthEditor*>(getParentComponent())) {
        auto& selectionModel = synthEditor->getSelectionModel();
        
        // Behandle die Selektion direkt
        if (!selectionModel.selectionContains(this)) {
            selectionModel.clearSelection();
            setSelected(true);
            selectionModel.getSelectedModules().push_back(this);
        }
        
        // Speichere Position für Verschiebung
        if (synthEditor->getCurrentLayer() == Module::Layer::GUI) {
            saveUiPosition();
        } else {
            savePosition();
        }
    }
}

void Knob::mouseDrag(const MouseEvent& e) {
    if (isDraggingSlider) {
        // Drag für Slider-Bedienung
        MouseEvent relativeEvent = e.getEventRelativeTo(slider);
        slider->mouseDrag(relativeEvent);
        return;
    }
    
    if (isDraggingModule && !locked && isSelected()) {
        // Direkte Verschiebung ohne Event-Weiterleitung
        if (auto* synthEditor = dynamic_cast<SynthEditor*>(getParentComponent())) {
            
            if (synthEditor->getCurrentLayer() == Module::Layer::GUI) {
                int x = getSavedUiPosition().getX() + e.getOffsetFromDragStart().getX();
                int y = getSavedUiPosition().getY() + e.getOffsetFromDragStart().getY();
                
                setUiPosition(x, y);
                setTopLeftPosition(x, y);
            } else {
                int x = getSavedPosition().getX() + e.getOffsetFromDragStart().getX();
                int y = getSavedPosition().getY() + e.getOffsetFromDragStart().getY();
                
                setTopLeftPosition(x, y);
            }
            
            // Repaint für sofortiges visuelles Feedback
            getParentComponent()->repaint();
        }
    }
}

void Knob::mouseUp(const MouseEvent& e) {
    if (isDraggingSlider) {
        // Event für Slider beenden
        MouseEvent relativeEvent = e.getEventRelativeTo(slider);
        slider->mouseUp(relativeEvent);
    }
    
    if (isDraggingModule && !locked) {
        // Kein Event-Handling nötig - die Verschiebung ist bereits komplett
    }
    
    // Zustand zurücksetzen
    isDraggingSlider = false;
    isDraggingModule = false;
}

void Knob::mouseMove(const MouseEvent& e) {
    if (locked) {
        setMouseCursor(MouseCursor::NormalCursor);
        isShowingSelectionHighlight = false;
        repaint();
        return;
    }
    
    bool wasShowingHighlight = isShowingSelectionHighlight;
    
    if (isInSliderArea(e.getPosition())) {
        // Über dem Slider - Rotary-Cursor für Bedienung
        setMouseCursor(MouseCursor::UpDownResizeCursor);
        isShowingSelectionHighlight = false;
    } else if (isInSelectionArea(e.getPosition())) {
        // Über dem Selektions-Bereich - Hand-Cursor für Verschiebung
        setMouseCursor(MouseCursor::DraggingHandCursor);
        isShowingSelectionHighlight = true;
    } else {
        // Außerhalb aller aktiven Bereiche
        setMouseCursor(MouseCursor::NormalCursor);
        isShowingSelectionHighlight = false;
    }
    
    // Repaint nur wenn sich der Highlight-Zustand geändert hat
    if (wasShowingHighlight != isShowingSelectionHighlight) {
        repaint();
    }
}

bool Knob::isInSelectionArea(const Point<int>& position) const {
    if (currentLayer == Module::Layer::ALL) {
        // ALL-Modus: Name-Label-Bereich rechts
        Rectangle<int> nameLabelArea(30, 0, 60, 30);
        return nameLabelArea.contains(position);
    } else {
        // GUI-Modus: Name-Label oben oder Rand-Bereiche
        Rectangle<int> nameLabelArea(0, 0, 90, 12);
        Rectangle<int> sliderArea = slider->getBounds();
        Rectangle<int> knobBounds = getLocalBounds();
        
        // Name-Label-Bereich oder außerhalb Slider aber innerhalb Knob
        return nameLabelArea.contains(position) || 
               (knobBounds.contains(position) && !sliderArea.contains(position));
    }
}

bool Knob::isInSliderArea(const Point<int>& position) const {
    return slider->getBounds().contains(position);
}

void Knob::mouseExit(const MouseEvent& e) {
    // Highlight und Cursor zurücksetzen wenn Maus die Knob verlässt
    if (isShowingSelectionHighlight) {
        isShowingSelectionHighlight = false;
        repaint();
    }
    setMouseCursor(MouseCursor::NormalCursor);
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
