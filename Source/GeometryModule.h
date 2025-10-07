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
class GeometryModule : public Module
{
public:
    GeometryModule();
    ~GeometryModule();

    void paint(juce::Graphics& g) override;
    virtual void process() override;
    virtual void configurePins() override;

    virtual juce::String getCategory() override {
        return "Math";
    }

private:

    juce::Vector3D<float> vector;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GeometryModule)
};
