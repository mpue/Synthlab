/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic startup code for a JUCE application.

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "MainComponent.h"
#include "Project/Project.h"
#include "AudioManager.h"
#include "Plugins/PluginManager.h"
#include <gtest.h>
#include "HTTPServer.h"
using juce::JUCEApplication;
using juce::String;
using juce::Logger;
using juce::DocumentWindow;
using juce::ScopedPointer;
using juce::Desktop;
using juce::Rectangle;

//==============================================================================
class SynthlabApplication  : public JUCEApplication
{
public:
    //==============================================================================
    SynthlabApplication() {}

    const String getApplicationName() override       { return ProjectInfo::projectName; }
    const String getApplicationVersion() override    { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed() override       { return true; }

    //==============================================================================
    void initialise (const String& commandLine) override
    {
        // This method is where you should put your application's initialisation code..

        Logger::writeToLog(commandLine);
        
        server = new HTTPServer(8080);
        server->startThread();
        
        if (commandLine.contains("runTests")) {
            //juce::UnitTestRunner runner;
            //runner.runAllTests();
        }
        else if(commandLine.contains("runUnitTests"))
        {
            int argc = 1;
            char* args = (char*)"";
            testing::InitGoogleTest(&argc, &args);
            (void) RUN_ALL_TESTS();
            juce::JUCEApplication::quit();
        }
        else {
            mainWindow = new MainWindow (getApplicationName());    
        }
        
    }

    void shutdown() override
    {
        if (mainWindow != nullptr)
            mainWindow->setLookAndFeel(nullptr);
         delete mainWindow; // (deletes our window)
#if defined(JUCE_PLUGINHOST_AU) || defined(JUCE_PLUGINHOST_VST)
        PluginManager::getInstance()->destroy();
#endif
        AudioManager::getInstance()->destroy();
        
        server->stop();
        server->stopThread(2000);
        delete server;
    }

    //==============================================================================
    void systemRequestedQuit() override
    {
        if (Project::getInstance()->isDirty()) {
            bool exit = juce::AlertWindow::showOkCancelBox(juce::AlertWindow::AlertIconType::NoIcon, "Hey!", "You have unsaved resources, wanna exit anyway?");
            
            if (exit) {
                quit();
            }
        }
        else {
            quit();
        }

    }

    void anotherInstanceStarted (const String& commandLine) override
    {
        // When another instance of the app is launched while this one is running,
        // this method is invoked, and the commandLine parameter tells you what
        // the other instance's command-line arguments were.
    }

    //==============================================================================
    /*
        This class implements the desktop window that contains an instance of
        our MainComponent class.
    */
    class MainWindow    : public DocumentWindow
    {
    public:
        MainWindow (String name)  : DocumentWindow (name,
                                                    Desktop::getInstance().getDefaultLookAndFeel()
                                                                          .findColour (ResizableWindow::backgroundColourId),
                                                    DocumentWindow::allButtons)
        {
            // Project::getInstance()->getLookAndFeel()->setColourScheme(LookAndFeel_V4::getDarkColourScheme());
            
            LookAndFeel::setDefaultLookAndFeel(Project::getInstance()->getLookAndFeel());
            
            // setLookAndFeel(Project::getInstance()->getLookAndFeel());
            setUsingNativeTitleBar (false);
            setContentOwned (new MainComponent(), true);
            setResizable (true, true);

            Rectangle<int> r = Desktop::getInstance().getDisplays().getMainDisplay().userArea;
	        centreWithSize (r.getWidth(), r.getHeight());
            setVisible (true);
        }

        void closeButtonPressed() override {
            
            if (Project::getInstance()->isDirty()) {
                bool exit = juce::AlertWindow::showOkCancelBox(juce::AlertWindow::AlertIconType::NoIcon, "Hey!", "You have unsaved resources, wanna exit anyway?");
                
                if (exit) {
                    quit();
                }
            }
            else {
                quit();
            }
 
        }

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
    };

private:
    MainWindow* mainWindow = nullptr;
    HTTPServer* server = nullptr;
};

//==============================================================================
// This macro generates the main() routine that launches the app.
START_JUCE_APPLICATION (SynthlabApplication)
