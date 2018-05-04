/*
  ==============================================================================

    ModuleTest.cpp
    Created: 3 May 2018 9:46:08pm
    Author:  Nils Schwenkel

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "../Project.h"
#include "../AudioManager.h"
#include "../Plugins/PluginManager.h"
#include "../MainComponent.h"


class ModuleTest : public juce::UnitTest {
    
public:
    ModuleTest() : juce::UnitTest("ModuleTest") {
        
    }
    
    void runTest() override {
        
        
        beginTest("asdf");
        Module* pModule = new Module();
        std::vector<Pin*>& pins = pModule->getPins();
        
        expectEquals<size_t>(0u, pins.size());
    }
    
    void newModule()
    {
        beginTest("newModule");
        Module* pModule = new Module();
        
        expect(nullptr != pModule);
        delete pModule;
    }
    
};

static ModuleTest moduleTest;
