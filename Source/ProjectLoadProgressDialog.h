#pragma once

#include <JuceHeader.h>

// Synchronous progress dialog used by SynthEditor::openFile.
// The project load runs on the message thread (plugin APIs require it) and
// holds the audio graph lock for its full duration, so the message loop can't
// dispatch normally. setProgress() flushes only pending paints via the peer —
// no other events fire, so ChangeBroadcaster listeners can't observe a
// half-built module tree.
class ProjectLoadProgressDialog : public juce::Component
{
public:
    ProjectLoadProgressDialog() : progressBar(displayedProgress)
    {
        progressBar.setPercentageDisplay(true);
        progressBar.setColour(juce::ProgressBar::foregroundColourId, juce::Colours::chartreuse);
        addAndMakeVisible(progressBar);
    }

    void resized() override
    {
        progressBar.setBounds(getLocalBounds().reduced(20));
    }

    void setProgress(double p)
    {
        displayedProgress = juce::jlimit(0.0, 1.0, p);
        progressBar.repaint();
        if (auto* peer = getPeer())
            peer->performAnyPendingRepaintsNow();
    }

private:
    double displayedProgress = 0.0;
    juce::ProgressBar progressBar;
};
