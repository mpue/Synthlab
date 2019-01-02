/*
 ==============================================================================
 
 MidiGate.h
 Created: 4 Apr 2018 3:48:23pm
 Author:  Matthias Pueski
 
 ==============================================================================
 */

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"

//==============================================================================
/*
 */
class ImageModule    : public Module
{
public:
    ImageModule();
    ~ImageModule();
    
    void paint (juce::Graphics& g) override;
    
    void setImage(String path);
    String getImage();
    
    String getPath() {
        return this->imagePath;
    }
    
    void setPath(String path) {
        this->imagePath = path;
    }
    
    virtual void configurePins() override;
    
    virtual juce::String getCategory() override {
        return "Math";
    }
    
    virtual Layer getLayer() override {
        return Layer::GUI;
    };
    
    void resized() override;
    
    struct ImageListener : juce::Value::Listener
    {
        ImageListener (juce::Value& v, ImageModule* c) : c(c),  value (v) { value.addListener (this); }
        ~ImageListener()  {}  // no need to remove the listener
        
        void valueChanged (juce::Value& value) override {
            c->setImage(value.toString());
        }
        ImageModule* c;
        juce::Value value;
    };
    
    struct ImagePathListener : juce::Value::Listener
    {
        ImagePathListener (juce::Value& v, ImageModule* c) : c(c),  value (v) { value.addListener (this); }
        ~ImagePathListener()  {}  // no need to remove the listener
        
        void valueChanged (juce::Value& value) override {
            
        }
        ImageModule* c;
        juce::Value value;
    };
    
    virtual juce::Array<juce::PropertyComponent*>& getProperties() override;
    virtual void createProperties() override;
    
private:
    
    juce::Value* imageValue;
    juce::PropertyComponent* imageProp;
    ImageListener* imageListener;
    juce::Value* imagePathValue;
    juce::PropertyComponent* imagePathProp;
    ImagePathListener* imagePathListener;
    Image* image = nullptr;
    Image* originalImage = nullptr;
    ResizableCornerComponent* resizer = nullptr;
    juce::String imagePath;
    bool editing = false;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ImageModule)
};
