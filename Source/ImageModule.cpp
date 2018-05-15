/*
 ==============================================================================
 
 ImageModule
 Created: 4 Apr 2018 3:48:23pm
 Author:  Matthias Pueski
 
 ==============================================================================
 */

#include "../JuceLibraryCode/JuceHeader.h"
#include "ImageModule.h"
#include "FileButtonPropertyComponent.h"

using juce::Justification;
using juce::Value;
using juce::PropertyComponent;
using juce::SliderPropertyComponent;
using juce::Array;
using juce::String;

//==============================================================================
ImageModule::ImageModule()
{
    
    setSize(120,30);
    nameLabel->setJustificationType (Justification::left);
    nameLabel->setTopLeftPosition(2,2);
    editable = false;
    prefab = true;
    
    createProperties();
    resizer = new ResizableCornerComponent(this,nullptr);
    resizer->setSize(getWidth(), getHeight());
    addAndMakeVisible(resizer);
    resizer->toFront(true);
    resizer->setTopLeftPosition(getWidth()-10, getHeight()-10);
    resizer->setSize(10,10);
    setInterceptsMouseClicks(false, true);
    // addMouseListener(resizer, true);
    //addMouseListener(resizer, true);
}

void ImageModule::createProperties() {
    imageValue = new Value();
    imageListener = new ImageListener(*imageValue, this);
}

ImageModule::~ImageModule()
{
    delete imageValue;
    delete imageListener;
    
    if (image != nullptr) {
        delete image;
    }
    
    delete resizer;
}

void ImageModule::configurePins(){
}

void ImageModule::paint(juce::Graphics &g) {
    Module::paint(g);
    if (this->image != nullptr) {
        if (image->getWidth() > 0 && image->getHeight() > 0) {
            g.drawImageAt(*image, 0, 0);
        }
    }
    if (selected) {
        g.setColour(Colours::orange);
        g.drawRect(0,0,getWidth(),getHeight());
    }
}

void ImageModule::resized() {
    if (resizer != nullptr) {
        resizer->setTopLeftPosition(getWidth() - 10, getHeight() - 10);
        
        if (originalImage != nullptr && this->image != nullptr) {
            if (getWidth() > 0 && getHeight() > 0) {
                this->image = new Image(this->originalImage->rescaled(getWidth(), getHeight()));
            }
        }
    }
}

String ImageModule::getImage() {
    return imagePath;
}

void ImageModule::setImage(String imagePath) {
    File imageFile = File(imagePath);
    Image img = ImageFileFormat::loadFrom(imageFile);
    this->image = new Image(img);
    this->imagePath = imagePath;
    
    originalImage = this->image;
    
    setSize(this->image->getWidth(), this->image->getHeight());
}

juce::Array<PropertyComponent*>& ImageModule::getProperties() {
    
    properties = juce::Array<PropertyComponent*>();
    imageProp = new FileButtonPropertyComponent("Select image", *imageValue);

    properties.add(imageProp);
    
    return properties;
}

