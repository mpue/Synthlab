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
#include "Module.h"
#include "Connection.h"
#include "AudioIn.h"
#include "AudioOut.h"
#include "AuxOut.h"
#include "AudioModule.h"
#include "SampleEditor.h"
#include "SelectionModel.h"
#include "StepSequencerModule.h"
#include "PrefabFactory.h"
#include <vector>
#include "MixerPanel.h"
#include "Project.h"

class SamplerModule;

//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Projucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class SynthEditor  : public Component,
                     public ChangeBroadcaster,
                     public AudioModule,
                     public Timer,
                     public DragAndDropTarget
         

{
public:
    //==============================================================================
    SynthEditor (double sampleRate, int buffersize );
    SynthEditor ();
    ~SynthEditor();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.

	void addConnection(const MouseEvent& e, Module* m);
    void deleteSelected(bool deleteAll);

    void setTab(TabbedComponent* t);
    
    void setModule(Module * m);
    Module* getModule();
    void cleanUp();
    void newFile();
    void saveFile();
    void openFile();

    void itemDropped (const SourceDetails& dragSourceDetails) override;
    
    bool isInterestedInDragSource (const SourceDetails& dragSourceDetails) override {
        return true;
    }
    
    void openSampleEditor(SamplerModule* sm);
    void openStepSequencer(StepSequencerModule* ssm);
    
    void saveStructure(std::vector<Module*>* modules, std::vector<Connection*>* connections, ValueTree* v);
    void loadStructure(std::vector<Module*>* modules, std::vector<Connection*>* connections,ValueTree* v);

    std::vector<AudioIn*>& getInputChannels();
    std::vector<AudioOut*>& getOutputChannels();
    std::vector<AuxOut*>& getAuxChannels();
    
    void removeSelectedItem();
    
    void setSamplerate(double rate);
    void setBufferSize(int buffersize);

    bool connectionExists(std::vector<Connection*> connections, Connection* c);
    void removeModule(Module* module);
    
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate);
    void showContextMenu(Point<int> position);
    bool channelIsValid(int channel);
    bool auxChannelIsValid(int channel, int subchannel);
    
    void loadFromString(String in);
    
    virtual float getSampleRate() override {
        return _sampleRate;
    }
    
    virtual float getBufferSize() override{
        return bufferSize;
    }
    
    bool isRunning() {
        return running;
        //stopTimer();
    }
    
    void setRunning(bool running) {
        this->running = running;
        // startTimer(100);
    }
    
    void timerCallback() override {
        repaint();
    }
    
    void setCurrentLayer(int layer) {
        
        currentLayer = static_cast<Module::Layer>(layer);
        
        for (int i = 0;i < root->getModules()->size();i++) {
            if (currentLayer == Module::Layer::ALL || root->getModules()->at(i)->getLayer() == currentLayer) {
                root->getModules()->at(i)->setVisible(true);
            }
            else {
                root->getModules()->at(i)->setVisible(false);
            }
        }
         
    }
    
    SelectionModel* getSelectionModel() const {
        return selectionModel;
    }
    
    void setMixer(MixerPanel* mixer) {
        this->mixer = mixer;
    }
    
    MixerPanel* getMixer() {
        return mixer;
    }
    
    void addChannel(String name, Mixer::Channel::Type channeltype);
    
    //[/UserMethods]

    void paint (Graphics& g) override;
    void resized() override;
    void mouseMove (const MouseEvent& e) override;
    void mouseDown (const MouseEvent& e) override;
    void mouseDrag (const MouseEvent& e) override;
    void mouseUp (const MouseEvent& e) override;
    void mouseDoubleClick (const MouseEvent& e) override;
    bool keyPressed (const KeyPress& key) override;
    bool keyStateChanged (bool isKeyDown) override;
    void modifierKeysChanged (const ModifierKeys& modifiers) override;
    
    
    
private:
    //[UserVariables]   -- You can add your own custom variables in this section.

    bool isAltDown = false;
    bool isCtrlDown = false;
    bool isLeftShiftDown = false;
    bool isLeftMouseDown = false;
	bool isMovingModule = true;
	bool isDraggingConnection = false;

    Module* root = nullptr;
    
    bool isRoot = true;;

	int mouseX = 0;
	int mouseY = 0;

    int lineStartX = 0;
    int lineStartY = 0;
    int lineStopX = 0;
    int lineStopY = 0;

	int dragStartX = 0;
	int dragStartY = 0;
    
    int mouseDownX = 0;
    int mouseDownY = 0;

    int dragDistanceX = 0;
    int dragDistanceY = 0;
    
    
	int maxrepaintw = 0;
	int maxrepainth = 0;
	int minrepaintx = 0;
	int minrepainty = 0;

    Pin* startPin = nullptr;

    SelectionModel::State state = SelectionModel::NONE;
    TabbedComponent* tab;
    Rectangle<int> selectionFrame = Rectangle<int>(0, 0, 0, 0);

    std::vector<AudioOut*> outputChannels;
    std::vector<AudioIn*> inputChannels;
    std::vector<AuxOut*> auxChannels;
    
    static String defaultMidiInputName;
    static String defaultMidiOutputName;
    
    int currentSample = 0;
    int bufferSize;
    double _sampleRate;
    bool running = false;
    bool locked = false;
    bool dragHasStarted = false;
    Module::Layer currentLayer = Module::Layer::ALL;
    
    SelectionModel* selectionModel;
  
    MixerPanel* mixer = nullptr;
    
    
    //[/UserVariables]

    
    //==============================================================================


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SynthEditor)
};

//[EndFile] You can add extra defines here...
//[/EndFile]
