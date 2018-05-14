/*
  ==============================================================================

  This is an automatically generated GUI class created by the Projucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Projucer version: 5.2.1

  ------------------------------------------------------------------------------

  The Projucer is part of the JUCE library.
  Copyright (c) 2017 - ROLI Ltd.

  ==============================================================================
*/

#pragma once

//[Headers]     -- You can add your own extra header files here --
#include "../JuceLibraryCode/JuceHeader.h"
#include "MultiComponentDragger.h"
#include <vector>
#include "Pin.h"
#include "AudioEngine/EventListener.h"
#include "AudioEngine/AudioProcessorModule.h"
#include "PropertyView.h"
// #include "Connection.h"

class Connection;

//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Projucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class Module  : public juce::Component,
                public juce::TextEditor::Listener,
                public EventListener,
                public juce::ChangeListener,
                public juce::ChangeBroadcaster
{
public:
    //==============================================================================

    Module();
    // Module(const Module& mod);
    virtual ~Module();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
    
    enum Layer {
        ALL,
        GUI,
        MODULES,
        CONNECTIONS
    };
    
    void addPin(Pin::Direction direction);
    void addPin(Pin::Direction direction, Pin *p);
    void addPin(Pin* p);
	Module(juce::String name);

    Pin* getSelectedPin();

    std::vector<Pin*> pins;

	bool isMouseOverPin(int pin, juce::Point<int> pos);

	juce::Point<int> getPinPosition(int i);

    virtual void setSelected(bool selected);
    bool isSelected();

	long getIndex();
	void setIndex(long index);
	virtual void setEditing(bool editing);

    std::vector<Connection*>* getConnections();
    std::vector<Module*>* getModules();
    virtual std::vector<Pin*>& getPins();
    
    virtual Pin* getValueOutputPin();

    void setConnections(std::vector<Connection*>* connections);
    void textEditorReturnKeyPressed(juce::TextEditor &) override;
    void setModules(std::vector<Module*>* modules);
    virtual void setName(juce::String name);

    bool isEditable() { return editable;}
    bool isPrefab() { return prefab;};

    void eventReceived(Event* e) override;
    void changeListenerCallback (juce::ChangeBroadcaster* source) override;
    virtual void process();
    
    void savePosition();
    void saveUiPosition();
    void moveTo(int x, int y);
    void setUiPosition(int x, int y);
    juce::Point<int> getSavedPosition();
    juce::Point<int> getSavedUiPosition();
    juce::Point<int> getUiPosition();
    
    virtual int getId() { return id;};
    virtual void setId(int id) {this->id = id;};
    
    virtual void configurePins() {};
    
    virtual juce::String getCategory();
    virtual juce::String getDescription();
    
    virtual void setSampleRate(float rate);
    virtual void setBuffersize(int buffersize);
    
    virtual Layer getLayer() {
        return Layer::MODULES;
    };
    
    virtual void setCurrentLayer(Layer layer){
        currentLayer = layer;
    }

    struct NameListener : juce::Value::Listener
    {
        NameListener (juce::Value& v, Module* module) : module(module),  value (v) { value.addListener (this); }
        ~NameListener()  {}  // no need to remove the listener
        
        void valueChanged (juce::Value& value) override {
            module->setName(value.toString());
        }
        Module* module;
        juce::Value value;
    };
    
    
    virtual juce::Array<juce::PropertyComponent*>& getProperties();
    virtual void createProperties();
    
    //[/UserMethods]

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseDoubleClick (const juce::MouseEvent& e) override;
    
    // Binary resources:
    static const char* materialicons_301_viewmodule_64_0_ffffff_none_png;
    static const int materialicons_301_viewmodule_64_0_ffffff_none_pngSize;

    void resetUIPosition() {
        uiPosition = getPosition();
    }

    
protected:
    //[UserVariables]   -- You can add your own custom variables in this section.
    MultiComponentDragger* dragger;

    NameListener* nameListener;
    juce::Value* nameValue;

    juce::PropertyComponent* nameProp;
    juce::Array<juce::PropertyComponent*> properties;
    
    bool selected = false;
	long index;
    bool editable = true;
    bool prefab = false;
    
    std::vector<Module*>* modules = nullptr;
    std::vector<Connection*>* connections = nullptr;
    Pin* selectedPin = nullptr;
    
    juce::Point<int> savedPosition;
    juce::Point<int> savedUIPosition;
    juce::Point<int> uiPosition;
    
    float sampleRate;
    int buffersize;
    
    juce::DropShadow* shadow = nullptr;
    juce::DropShadower* shadower = nullptr;
    
    Layer currentLayer = Layer::ALL;
    
    //[/UserVariables]


    
    //==============================================================================
    juce::ScopedPointer<juce::TextEditor> nameEditor;
    juce::ScopedPointer<juce::Label> nameLabel;
    juce::Image cachedImage_materialicons_301_viewmodule_64_0_ffffff_none_png_1;

    int id = 0;
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Module)
};

//[EndFile] You can add extra defines here...
//[/EndFile]
