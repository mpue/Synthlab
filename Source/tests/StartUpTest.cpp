/*
  ==============================================================================

    StartUpTest.cpp
    Created: 26 Apr 2018 9:48:20am
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "../Project.h"
#include "../AudioManager.h"
#include "../Plugins/PluginManager.h"
#include "../MainComponent.h"
#include "../PrefabFactory.h"

class StartupTest : public juce::UnitTest {
    
public:
    StartupTest() : juce::UnitTest("Startup") {
        
    }
    
    void runTest() override {
        beginTest("Init");
        expect(Project::getInstance() != nullptr);
        expect(AudioManager::getInstance() != nullptr);
        expect(PluginManager::getInstance() != nullptr);
    }
    
};

static StartupTest startupTest;

