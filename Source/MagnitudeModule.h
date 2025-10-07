/*
  ==============================================================================

    MagnitudeModule.h
	Created: today 12:42:07pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#pragma once


#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"

//==============================================================================
/*
 */
class MagnitudeModule : public Module
{
public:
    MagnitudeModule();
    ~MagnitudeModule();

    void paint(juce::Graphics& g) override;
    virtual void process() override;
    virtual void configurePins() override;

    virtual juce::String getCategory() override {
        return "Audio";
    }

	// return the magnitude value

    float getMagnitude() {
        return magnitude;
	}

private:

	float magnitude = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MagnitudeModule)
};
