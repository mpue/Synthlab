#include "../JuceLibraryCode/JuceHeader.h"

class MainComponent;

class BounceThread : public juce::Thread
{
public:
    BounceThread(MainComponent* mc, const juce::File& file, double duration, std::function<void(float)> progressCallback)
        : Thread("BounceThread"), main(mc), outputFile(file), durationInSeconds(duration), onProgress(progressCallback) {
    }
	void run() override;

private:
    MainComponent* main;
    juce::File outputFile;
    double durationInSeconds;
    std::function<void(float)> onProgress;
};