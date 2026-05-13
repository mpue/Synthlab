/*
  ==============================================================================

    CrashHandler.h

    Registers a global crash handler with JUCE that dumps a timestamped
    stack trace + system info to ~/Library/Application Support/Synthlab/Crashes
    (or the equivalent userApplicationDataDirectory on other platforms).

    Two-stage user notification:
      1. In-process: if the crash happens on the message thread, attempt a
         native OS alert immediately so the user sees something before the
         process dies. Skipped if the crashing thread is not the message
         thread (NSAlert/MessageBox are not safe to call from arbitrary
         threads in a signal context).
      2. Next-start: checkForPreviousCrashes() scans the crash directory
         for files newer than a marker timestamp and offers to reveal them.

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

    static juce::File getStartupMarker()
    {
        return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                   .getChildFile ("Synthlab")
                   .getChildFile ("last-startup");
    }

    static void install()
    {
        getCrashDir().createDirectory();
        juce::SystemStats::setApplicationCrashHandler (&CrashHandler::handle);
    }

    // Call from Main::initialise() — after the logger is installed but
    // before the main window appears. Shows a single Ok/Cancel dialog if
    // crash reports newer than the previous startup marker exist, then
    // refreshes the marker so we don't re-prompt on the next clean run.
    static void checkForPreviousCrashes()
    {
        auto marker = getStartupMarker();
        const juce::Time since = marker.existsAsFile()
                                     ? marker.getLastModificationTime()
                                     : juce::Time (0);

        juce::Array<juce::File> recent;
        for (auto& f : getCrashDir().findChildFiles (juce::File::findFiles, false, "crash-*.log"))
            if (f.getLastModificationTime() > since)
                recent.add (f);

        // Always bump the marker so a single crash is reported at most once.
        marker.replaceWithText (juce::Time::getCurrentTime().toISO8601 (true));

        if (recent.isEmpty())
            return;

        // Sort newest-first so the "reveal" target is the latest crash.
        std::sort (recent.begin(), recent.end(),
                   [] (const juce::File& a, const juce::File& b)
                   { return a.getLastModificationTime() > b.getLastModificationTime(); });

        juce::String msg;
        msg << "Synthlab did not shut down cleanly the last time it ran.\n\n"
            << recent.size() << " crash report"
            << (recent.size() == 1 ? "" : "s")
            << " were saved to:\n"
            << getCrashDir().getFullPathName() << "\n\n"
            << "Most recent: " << recent.getFirst().getFileName() << "\n\n"
            << "Reveal in Finder?";

        const bool reveal = juce::AlertWindow::showOkCancelBox (
            juce::AlertWindow::WarningIcon,
            "Previous crash detected",
            msg,
            "Reveal", "Dismiss",
            nullptr, nullptr);

        if (reveal)
            recent.getFirst().revealToUser();
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

        // Only attempt an in-process dialog when we're on the message
        // thread. Calling NSAlert/MessageBox from the audio thread inside
        // a signal handler is the textbook recipe for a deadlock; in that
        // case we rely on checkForPreviousCrashes() at next launch.
        if (juce::MessageManager::getInstanceWithoutCreating() != nullptr
            && juce::MessageManager::getInstance()->isThisTheMessageThread())
        {
            juce::NativeMessageBox::showMessageBox (
                juce::MessageBoxIconType::WarningIcon,
                "Synthlab crashed",
                "An unexpected error occurred and Synthlab has to close.\n\n"
                "A crash report was saved to:\n"
                + file.getFullPathName()
                + "\n\nIt will also be offered when you next start Synthlab.");
        }
    }
};
