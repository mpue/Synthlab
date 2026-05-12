/*
  ==============================================================================

    CrashHandler.h

    Registers a global crash handler with JUCE that dumps a timestamped
    stack trace + system info to ~/Library/Application Support/Synthlab/Crashes
    (or the equivalent userApplicationDataDirectory on other platforms).

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

class CrashHandler
{
public:
    static juce::File getCrashDir()
    {
        return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                   .getChildFile ("Synthlab")
                   .getChildFile ("Crashes");
    }

    static void install()
    {
        getCrashDir().createDirectory();
        juce::SystemStats::setApplicationCrashHandler (&CrashHandler::handle);
    }

private:
    static void handle (void* /*platformSpecificData*/)
    {
        const auto stamp = juce::Time::getCurrentTime().formatted ("%Y%m%d-%H%M%S");
        auto file = getCrashDir().getChildFile ("crash-" + stamp + ".log");

        juce::String text;
        text << "Synthlab crash report\n"
             << "Time:   " << juce::Time::getCurrentTime().toString (true, true, true, true) << "\n"
             << "App:    " << juce::String (ProjectInfo::projectName)
                            << " " << juce::String (ProjectInfo::versionString) << "\n"
             << "OS:     " << juce::SystemStats::getOperatingSystemName() << "\n"
             << "CPU:    " << juce::SystemStats::getCpuModel() << "\n"
             << "Cores:  " << juce::SystemStats::getNumCpus() << "\n"
             << "Memory: " << juce::SystemStats::getMemorySizeInMegabytes() << " MB\n\n"
             << "Backtrace:\n"
             << juce::SystemStats::getStackBacktrace() << "\n";

        file.replaceWithText (text);
    }
};
