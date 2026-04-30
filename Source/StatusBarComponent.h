/*
  ==============================================================================

    StatusBarComponent.h
    Header-only Statuszeile für das Hauptfenster.
    Zeigt CPU-Last, Process-Memory, Audio-Setup und Tempo.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#if JUCE_MAC
 #include <mach/mach.h>
#elif JUCE_LINUX
 #include <unistd.h>
 #include <cstdio>
#elif JUCE_WINDOWS
 #include <windows.h>
 #include <psapi.h>
 #pragma comment(lib, "psapi.lib")
#endif

class StatusBarComponent : public juce::Component
{
public:
    StatusBarComponent()
    {
        for (auto* lbl : { &cpuLabel, &memLabel, &audioLabel, &tempoLabel })
        {
            lbl->setFont(juce::Font(11.0f, juce::Font::plain));
            lbl->setColour(juce::Label::textColourId, juce::Colour(0xffbbbbbb));
            lbl->setJustificationType(juce::Justification::centredLeft);
            addAndMakeVisible(lbl);
        }

        cpuLabel.setText  ("CPU: --",          juce::dontSendNotification);
        memLabel.setText  ("RAM: --",          juce::dontSendNotification);
        audioLabel.setText("-- kHz / --",      juce::dontSendNotification);
        tempoLabel.setText("Tempo: --",        juce::dontSendNotification);
    }

    void paint(juce::Graphics& g) override
    {
        // Leicht abgesetzt vom Editor-Hintergrund, damit die Statusleiste
        // visuell vom Inhalt darüber getrennt ist.
        g.fillAll(juce::Colour(0xff1a1a1a));
        g.setColour(juce::Colour(0xff333333));
        g.fillRect(0, 0, getWidth(), 1);
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced(8, 2);
        const int colW = juce::jmax(110, r.getWidth() / 4);

        cpuLabel  .setBounds(r.removeFromLeft(colW));
        memLabel  .setBounds(r.removeFromLeft(colW));
        audioLabel.setBounds(r.removeFromLeft(colW));
        tempoLabel.setBounds(r);
    }

    void setCpuLoad(int percent)
    {
        cpuLabel.setText("CPU: " + juce::String(percent) + " %", juce::dontSendNotification);
    }

    void setMemoryUsageMB(int mb)
    {
        if (mb < 0)
            memLabel.setText("RAM: --", juce::dontSendNotification);
        else
            memLabel.setText("RAM: " + juce::String(mb) + " MB", juce::dontSendNotification);
    }

    void setAudioInfo(double sampleRate, int bufferSize)
    {
        audioLabel.setText(juce::String(sampleRate / 1000.0, 1) + " kHz / "
                           + juce::String(bufferSize), juce::dontSendNotification);
    }

    void setTempo(float bpm)
    {
        tempoLabel.setText("Tempo: " + juce::String(bpm, 1), juce::dontSendNotification);
    }

    // Resident Set Size in MB des aktuellen Prozesses, plattform-spezifisch.
    // -1 wenn nicht ermittelbar (kein Crash, Statusleiste zeigt dann "--").
    static int getProcessMemoryUsageMB()
    {
       #if JUCE_MAC
        mach_task_basic_info_data_t info;
        mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
        if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                      reinterpret_cast<task_info_t>(&info), &count) == KERN_SUCCESS)
            return (int)(info.resident_size / (1024 * 1024));
        return -1;
       #elif JUCE_LINUX
        long rss = 0;
        if (FILE* fp = std::fopen("/proc/self/statm", "r"))
        {
            long total = 0;
            if (std::fscanf(fp, "%ld %ld", &total, &rss) != 2) rss = 0;
            std::fclose(fp);
            const long pageSize = sysconf(_SC_PAGESIZE);
            return (int)((rss * pageSize) / (1024 * 1024));
        }
        return -1;
       #elif JUCE_WINDOWS
        PROCESS_MEMORY_COUNTERS pmc{};
        if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
            return (int)(pmc.WorkingSetSize / (1024 * 1024));
        return -1;
       #else
        return -1;
       #endif
    }

private:
    juce::Label cpuLabel, memLabel, audioLabel, tempoLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StatusBarComponent)
};
