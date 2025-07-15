#include <JuceHeader.h>

class BounceProgressDialog : public juce::Component, public juce::Timer
{
public:
    BounceProgressDialog(std::atomic<double>& sourceProgress, std::function<bool()> isDone)
        : source(sourceProgress), isFinished(isDone), progressBar(displayedProgress)
    {
        progressBar.setPercentageDisplay(true);
        progressBar.setColour(juce::ProgressBar::foregroundColourId, juce::Colours::chartreuse);
        addAndMakeVisible(progressBar);
        startTimer(100);
    }

    void resized() override
    {
        progressBar.setBounds(getLocalBounds().reduced(20));
    }

    void timerCallback() override
    {
        displayedProgress = source.load();  // copy atomic to double
        if (isFinished()) {
            stopTimer();
            if (auto* dw = findParentComponentOfClass<juce::DialogWindow>())
                dw->exitModalState(0);
        }
    }

private:
    std::atomic<double>& source;
    double displayedProgress = 0.0;
    std::function<bool()> isFinished;
    juce::ProgressBar progressBar;
};
