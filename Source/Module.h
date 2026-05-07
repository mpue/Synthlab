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

	void addConnection(Connection* c);
	void connectPins(int outputPinIndex, Pin* inputPin);

    void setConnections(std::vector<Connection*>* connections);
    void textEditorReturnKeyPressed(juce::TextEditor &) override;
    void setModules(std::vector<Module*>* modules);
    virtual void setName(juce::String name);

    bool isEditable() { return editable;}
    bool isPrefab() { return prefab;};

    void setResizable(bool r);
    bool isResizable() const { return resizer != nullptr; }

    void eventReceived(Event* e) override;
    void changeListenerCallback (juce::ChangeBroadcaster* source) override;
    virtual void process();
    
    virtual void process(MidiBuffer* externalBuffer) {};
    virtual void processInternal(MidiBuffer* externalBuffer, int numSamples) {};
    virtual void process(MidiBuffer* externalBuffer, int numSamples) {};

    void savePosition();
    void saveUiPosition();
    void moveTo(int x, int y);
    void setUiPosition(int x, int y);
    juce::Point<int> getSavedPosition();
    juce::Point<int> getSavedUiPosition();
    juce::Point<int> getUiPosition();
    
    virtual int getId() { return id;};
    virtual void setId(int id) {this->id = id;};
    
    virtual juce::String getUuid() { return uuid;};
    virtual void setUuid(juce::String uuid) {this->uuid = uuid;};
    
    virtual void configurePins() {};
    
    virtual juce::String getCategory();
    virtual juce::String getDescription();
    
    virtual void setSampleRate(float rate);
    virtual void setBuffersize(int buffersize);
    double getSampleRate() const { return sampleRate; }
    int getBufferSize() const { return buffersize; }
    
    virtual void setLocked(bool locked) {
        this->locked = locked;
    }
    
    virtual Layer getLayer() {
        return Layer::MODULES;
    };
    
    virtual void setCurrentLayer(Layer layer){
        currentLayer = layer;
    }

    Layer getCurrentLayer() const { return currentLayer; }

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

    struct DisplayRootListener : juce::Value::Listener
    {
        DisplayRootListener(juce::Value& v, Module* module) : module(module), value(v) { value.addListener(this); }
        ~DisplayRootListener() {}

        void valueChanged(juce::Value& value) override {
            module->setDisplayAtRootLevel(value.toString().getIntValue() > 0);
        }
        Module* module;
        juce::Value value;
    };

    bool getDisplayAtRootLevel() const { return displayAtRootLevel; }
    void setDisplayAtRootLevel(bool val) {
        displayAtRootLevel = val;
        if (displayRootValue != nullptr)
            displayRootValue->setValue(val);
    }

    // Position used by the root-level "Controls only" surfacing. For a module
    // whose children are surfaced, this is the group's top-left in editor
    // coordinates. For a surfaced control, this is its position inside the
    // group. (-1, -1) means "no user-set position yet — use default layout".
    juce::Point<int> getSurfacePos() const { return surfacePos; }
    void setSurfacePos(juce::Point<int> p) { surfacePos = p; }
    bool hasSurfacePos() const { return surfacePos.x >= 0 && surfacePos.y >= 0; }

    // Size of the per-source RootLevelGroup at the editor root. Set when the
    // user drags the group's corner resizer; (0, 0) means "auto-fit".
    juce::Point<int> getSurfaceSize() const { return surfaceSize; }
    void setSurfaceSize(juce::Point<int> s) { surfaceSize = s; }
    bool hasSurfaceSize() const { return surfaceSize.x > 0 && surfaceSize.y > 0; }

    virtual juce::Array<juce::PropertyComponent*>& getProperties();
    virtual void createProperties();
    
    //[/UserMethods]

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseDoubleClick (const juce::MouseEvent& e) override;
    
    void setChannel(int channel) {
        this->channel = channel;
	}

    int getChannel() {
        return channel;
	}

    // Binary resources:
    static const char* materialicons_301_viewmodule_64_0_ffffff_none_png;
    static const int materialicons_301_viewmodule_64_0_ffffff_none_pngSize;

    void resetUIPosition() {
        uiPosition = getPosition();
        saveUiPosition();
    }

protected:
    //[UserVariables]   -- You can add your own custom variables in this section.
    MultiComponentDragger* dragger;

    NameListener* nameListener;
    juce::Value* nameValue;

    DisplayRootListener* displayRootListener = nullptr;
    juce::Value* displayRootValue = nullptr;
    bool displayAtRootLevel = false;
    juce::Point<int> surfacePos { -1, -1 };
    juce::Point<int> surfaceSize { 0, 0 };

    juce::PropertyComponent* nameProp;
    juce::Array <juce::PropertyComponent*> properties;

    bool selected = false;
	long index;
    bool editable = true;
    bool prefab = false;
    bool locked = false;
    
    std::vector<Module*>* modules = nullptr;
    std::vector<Connection*>* connections = nullptr;
    Pin* selectedPin = nullptr;
    
    juce::Point<int> savedPosition;
    juce::Point<int> savedUIPosition;
    juce::Point<int> uiPosition;
    
    // Safe defaults so per-block sample loops in modules that never get an
    // explicit setBuffersize/setSampleRate (e.g. TerminalModule via
    // PrefabFactory, or freshly-created group modules) still iterate. With
    // buffersize uninitialized (often 0) the AUDIO copy in TerminalModule
    // ran zero times and groups appeared silent.
    double sampleRate = 44100.0;
    int buffersize = 512;
    
    juce::DropShadow* shadow = nullptr;
    juce::DropShadower* shadower = nullptr;

    Layer currentLayer = Layer::ALL;

    juce::ResizableCornerComponent* resizer = nullptr;

    //[/UserVariables]

    //==============================================================================
    juce::ScopedPointer<juce::TextEditor> nameEditor;
    juce::ScopedPointer<juce::Label> nameLabel;
    juce::Image cachedImage_materialicons_301_viewmodule_64_0_ffffff_none_png_1;

    int id = 0;
    
    int channel;

    juce::String uuid;
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Module)
};

//[EndFile] You can add extra defines here...
//[/EndFile]
