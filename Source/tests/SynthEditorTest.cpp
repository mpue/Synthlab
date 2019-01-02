/*
  ==============================================================================

    LoadSynthTest.cpp
    Created: 26 Apr 2018 10:22:50am
    Author:  Matthias Pueski

  ==============================================================================
*/

/*
 ==============================================================================
 
 LoadSynthTest.h
 Created: 26 Apr 2018 10:19:59am
 Author:  Matthias Pueski
 
 ==============================================================================
 */

#include "../JuceLibraryCode/JuceHeader.h"
#include "../Project/Project.h"
#include "../AudioManager.h"
#include "../Plugins/PluginManager.h"
#include "../MainComponent.h"

using juce::File;
using juce::UnitTest;
using juce::Logger;
using juce::String;
using juce::JUCEApplication;

#include <gtest.h>

TEST(SynthEditorTest, DISABLED_load)
{
    ScopedPointer<MainComponent> mc = new MainComponent();
    ASSERT_NE(nullptr, AudioManager::getInstance()->getDeviceManager());
    ASSERT_NE(nullptr, mc->getEditor());
    ASSERT_NE(nullptr, mc->getEditor()->getMixer());
    
    File f = File(File::getCurrentWorkingDirectory().getFullPathName()+"/../../../../Regression/NoiseTest.xml");
    ASSERT_EQ(true, f.exists());
    
    Logger::writeToLog(f.getFullPathName());
    
    String data = f.loadFileAsString();
    
    mc->getEditor()->loadFromString(data);
    
    ASSERT_NE(nullptr,mc->getEditor()->getModule());
    ASSERT_EQ(3,mc->getEditor()->getModule()->getConnections()->size());
    ASSERT_EQ(3,mc->getEditor()->getModule()->getModules()->size());
    mc = nullptr;

    juce::JUCEApplication::quit();
    
}






