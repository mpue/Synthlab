/*
  ==============================================================================

    LoadSynthTest.h
    Created: 26 Apr 2018 10:19:59am
    Author:  Matthias Pueski

  ==============================================================================
*/

#pragma once

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "../Project.h"
#include "../AudioManager.h"
#include "../Plugins/PluginManager.h"
#include "../MainComponent.h"


class LoadSynthTest : public UnitTest {
    
public:
    LoadSynthTest() : UnitTest("LoadSynthTest") {
        
    }
    
    void runTest() override {
        beginTest("LoadSynth");

        MainComponent* mc = new MainComponent();
        expect(AudioManager::getInstance()->getDeviceManager() != nullptr);
        
        expect(mc->getEditor() != nullptr);
        expect(mc->getEditor()->getMixer() != nullptr);
        
        File f = File(File::getCurrentWorkingDirectory().getFullPathName()+"/../../../../Regression/two");
        Logger::writeToLog(f.getFullPathName());
        
        String data = f.loadFileAsString();
        
        mc->getEditor()->loadFromString(data);

        expect(mc->getEditor()->getModule() != nullptr);
        expect(mc->getEditor()->getModule()->getConnections()->size() == 20);
        expect(mc->getEditor()->getModule()->getModules()->size() == 16);
        
        
        delete mc;
        JUCEApplication::getInstance()->quit();
    
    }
    
};

static LoadSynthTest loadSynthTest;
