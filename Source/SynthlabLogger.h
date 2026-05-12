/*
  ==============================================================================

    SynthlabLogger.h

    File logger + in-memory ring buffer for the on-screen LogView.
    Install once in JUCEApplication::initialise(); all existing
    juce::Logger::writeToLog(...) calls then fan out to both the log file
    and the LogView (which polls drainSince()).

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include <deque>
#include <utility>

class SynthlabLogger : public juce::Logger
{
public:
    static SynthlabLogger& get()
    {
        static SynthlabLogger inst;
        return inst;
    }

    void install()
    {
        if (fileLogger == nullptr)
        {
            auto dir = getLogDir();
            dir.createDirectory();

            const juce::String welcome =
                "=== " + juce::String(ProjectInfo::projectName) + " "
                       + juce::String(ProjectInfo::versionString) + " — log opened "
                       + juce::Time::getCurrentTime().toString(true, true, true, true) + " ===";

            fileLogger.reset(new juce::FileLogger(dir.getChildFile("synthlab.log"),
                                                  welcome,
                                                  1024 * 1024));
        }

        juce::Logger::setCurrentLogger(this);
    }

    void uninstall()
    {
        if (juce::Logger::getCurrentLogger() == this)
            juce::Logger::setCurrentLogger(nullptr);
        fileLogger.reset();
    }

    juce::File getLogFile() const
    {
        return fileLogger != nullptr ? fileLogger->getLogFile() : juce::File();
    }

    static juce::File getLogDir()
    {
        return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                   .getChildFile("Synthlab")
                   .getChildFile("Logs");
    }

    // Pull all entries newer than seqInOut. Updates seqInOut to the latest seen.
    juce::StringArray drainSince(juce::int64& seqInOut)
    {
        const juce::ScopedLock sl(lock);
        juce::StringArray out;
        for (auto& entry : buffer)
        {
            if (entry.first > seqInOut)
            {
                out.add(entry.second);
                seqInOut = entry.first;
            }
        }
        return out;
    }

protected:
    void logMessage(const juce::String& message) override
    {
        const auto stamped = juce::Time::getCurrentTime().formatted("%H:%M:%S")
                             + "  " + message;

        if (fileLogger != nullptr)
            fileLogger->logMessage(stamped);

        const juce::ScopedLock sl(lock);
        buffer.emplace_back(++seq, stamped);
        while (buffer.size() > kMaxBuffered)
            buffer.pop_front();
    }

private:
    SynthlabLogger() = default;
    ~SynthlabLogger() override { uninstall(); }

    static constexpr size_t kMaxBuffered = 4000;

    std::unique_ptr<juce::FileLogger> fileLogger;
    juce::CriticalSection lock;
    std::deque<std::pair<juce::int64, juce::String>> buffer;
    juce::int64 seq = 0;

    JUCE_DECLARE_NON_COPYABLE (SynthlabLogger)
};
