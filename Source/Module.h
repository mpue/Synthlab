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
class Module  : public Component,
                public TextEditor::Listener,
                public EventListener,
                public ChangeListener,
                public ChangeBroadcaster
{
public:
    //==============================================================================
    Module ();
    ~Module();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
    void addPin(Pin::Direction direction);
    void addPin(Pin::Direction direction, Pin *p);
    void addPin(Pin* p);
	Module(String name);

    Pin* getSelectedPin();

    std::vector<Pin*> pins;

	bool isMouseOverPin(int pin, Point<int> pos);

	Point<int> getPinPosition(int i);

    void setSelected(bool selected);
    bool isSelected();

	long getIndex();
	void setIndex(long index);
	void setEditing(bool editing);

    std::vector<Connection*>* getConnections();
    std::vector<Module*>* getModules();
    std::vector<Pin*> getPins();

    void setConnections(std::vector<Connection*>* connections);
    void textEditorReturnKeyPressed(juce::TextEditor &) override;
    void setModules(std::vector<Module*>* modules);
    void setName(String name) ;

    bool isEditable() { return editable;}
    bool isPrefab() { return prefab;};

    void eventReceived(Event* e) override;
    void changeListenerCallback (ChangeBroadcaster* source);
    virtual void process();
    
    void savePosition();
    void moveTo(int x, int y);
    Point<int> getSavedPosition();
    
    virtual int getId() { return id;};
    virtual void setId(int id) {this->id = id;};
    
    virtual void configurePins() {};
    
    virtual String getCategory();
    
    //[/UserMethods]

    void paint (Graphics& g) override;
    void resized() override;
    void mouseDoubleClick (const MouseEvent& e) override;

    // Binary resources:
    static const char* materialicons_301_viewmodule_64_0_ffffff_none_png;
    static const int materialicons_301_viewmodule_64_0_ffffff_none_pngSize;


    
protected:
    //[UserVariables]   -- You can add your own custom variables in this section.
    MultiComponentDragger* dragger;

    bool selected = false;
	long index;
    bool editable = true;
    bool prefab = false;
    std::vector<Module*> modules;
    std::vector<Connection*> connections;
    Pin* selectedPin = nullptr;
    
    Point<int> savedPosition;
    //[/UserVariables]

    //==============================================================================
    ScopedPointer<TextEditor> nameEditor;
    ScopedPointer<Label> nameLabel;
    Image cachedImage_materialicons_301_viewmodule_64_0_ffffff_none_png_1;

    int id = 0;
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Module)
};

//[EndFile] You can add extra defines here...
//[/EndFile]
