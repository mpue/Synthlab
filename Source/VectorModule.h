/*
  ==============================================================================

    VectorModule.h
    Created: 6 Oct 2025 7:37:45pm
    Author:  mpue

  ==============================================================================
*/

#pragma once


#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"

//==============================================================================
/*
 */
class VectorModule : public Module
{
public:
    VectorModule();
    ~VectorModule();

    void paint(juce::Graphics& g) override;
    virtual void process() override;
    virtual void configurePins() override;

    virtual juce::String getCategory() override {
        return "Math";
    }
 
private:

	juce::Vector3D<float> vector;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VectorModule)
};
