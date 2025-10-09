/*
  ==============================================================================
    DonutModule.h
    Created: 7 Oct 2025
    Author:  mpue
  ==============================================================================
*/
#pragma once
#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"

//==============================================================================
/*
 */
class DonutModule : public Module
{
public:
    DonutModule();
    ~DonutModule();
    void paint(juce::Graphics& g) override;
    virtual void process() override;
    virtual void configurePins() override;
    virtual juce::String getCategory() override {
        return "Graphics";
    }

private:
    juce::Vector3D<float> translate;
    juce::Vector3D<float> scale;
    juce::Vector3D<float> rotate;

    std::unique_ptr<unsigned int[]> indices;
    std::unique_ptr<float[]> vertices;
    std::unique_ptr<Geometry> donut;

    // Donut-spezifische Parameter
    float majorRadius = 1.0f;  // Äußerer Radius
    float minorRadius = 0.4f;  // Röhrenradius
    int majorSegments = 32;     // Segmente um den Ring
    int minorSegments = 16;     // Segmente der Röhre

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DonutModule)
};