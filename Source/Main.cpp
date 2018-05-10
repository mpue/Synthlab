/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic startup code for a JUCE application.

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "MainComponent.h"
#include "Project.h"
#include "AudioManager.h"
#include "Plugins/PluginManager.h"
#include <gtest.h>

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
        mainWindow = nullptr; // (deletes our window)
        PluginManager::getInstance()->destroy();
        AudioManager::getInstance()->destroy();
        

    }

    //==============================================================================
    void systemRequestedQuit() override
    {
        // This is called when the app is being asked to quit: you can ignore this
        // request and let the app carry on running, or call quit() to allow the app to close.
        quit();
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
            setLookAndFeel(Project::getInstance()->getLookAndFeel());
            setUsingNativeTitleBar (false);
            setContentOwned (new MainComponent(), true);
            setResizable (true, true);

            Rectangle<int> r = Desktop::getInstance().getDisplays().getMainDisplay().userArea;
#if JUCE_MAC
			centreWithSize(r.getWidth(), r.getHeight() - 25);
#else
	        centreWithSize (r.getWidth(), r.getHeight());
#endif

            setVisible (true);
        }

        void closeButtonPressed() override
        {
            // This is called when the user tries to close this window. Here, we'll just
            // ask the app to quit when this happens, but you can change this to do
            // whatever you need.
            JUCEApplication::getInstance()->shutdown();
        }

        /* Note: Be careful if you override any DocumentWindow methods - the base
           class uses a lot of them, so by overriding you might break its functionality.
           It's best to do all your work in your content component instead, but if
           you really have to override any DocumentWindow methods, make sure your
           subclass also calls the superclass's method.
        */

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
    };

private:
    ScopedPointer<MainWindow> mainWindow = nullptr;
};

//==============================================================================
// This macro generates the main() routine that launches the app.
START_JUCE_APPLICATION (SynthlabApplication)
