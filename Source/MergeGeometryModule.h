/*
  ==============================================================================

    MergeGeometryModule.h
    Created: 9 Oct 2025 2:54:54pm
    Author:  mpue

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"
#include "Graphics/Geometry.h"

using namespace juce;

//==============================================================================
/*
 */
class MergeGeometryModule : public Module
{
public:
    MergeGeometryModule();
    ~MergeGeometryModule();

    void paint(juce::Graphics& g) override;
    virtual void process() override;
    virtual void configurePins() override;

    virtual juce::String getCategory() override {
        return "Geometry";
    }

    virtual juce::String getDescription() override {
        return "Merges two geometries into one";
    }

private:
    void mergeGeometries();

    Geometry* geometry1 = nullptr;
    Geometry* geometry2 = nullptr;
    Geometry* mergedGeometry = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MergeGeometryModule)
};