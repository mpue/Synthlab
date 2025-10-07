/*
  ==============================================================================

    BoxModule.h
    Created: 7 Oct 2025 2:41:48pm
    Author:  mpue

  ==============================================================================
*/

#pragma once
/*
  ==============================================================================

    CompareModule.h
    Created: 24 Aug 2018 12:53:21pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#pragma once


#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"

//==============================================================================
/*
 */
class BoxModule : public Module
{
public:
    BoxModule();
    ~BoxModule();

    void paint(juce::Graphics& g) override;
    virtual void process() override;
    virtual void configurePins() override;

    virtual juce::String getCategory() override {
        return "Math";
    }

private:

    juce::Vector3D<float> translate;
    juce::Vector3D<float> scale;
    juce::Vector3D<float> rotate;

	unsigned int* indices = nullptr;
	float* vertices = nullptr;

	Geometry* box = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BoxModule)
};
