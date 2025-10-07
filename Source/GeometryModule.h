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

using namespace juce;

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

    Geometry* getGeometry() {
        return geometry;
	}
  
private:

    juce::Vector3D<float> translate;
    juce::Vector3D<float> scale;
    juce::Vector3D<float> rotate;

	Geometry* geometry = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GeometryModule)
};
