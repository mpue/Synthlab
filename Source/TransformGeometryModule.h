/*
  ==============================================================================
    TransformGeometryModule.h
    Created: 9 Oct 2025
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
class TransformGeometryModule : public Module
{
public:
    TransformGeometryModule();
    ~TransformGeometryModule();

    void paint(juce::Graphics& g) override;
    virtual void process() override;
    virtual void configurePins() override;

    virtual juce::String getCategory() override {
        return "Geometry";
    }

    virtual juce::String getDescription() override {
        return "Transforms geometry with translation, rotation and scale";
    }

private:
    void transformGeometry();
    Matrix3D<float> createRotationMatrix(const Vector3D<float>& rotation);
    Vector3D<float> transformVertex(const Vector3D<float>& vertex,
        const Vector3D<float>& translation,
        const Vector3D<float>& rotation,
        const Vector3D<float>& scale);

    Geometry* inputGeometry = nullptr;
    Geometry* transformedGeometry = nullptr;

    Vector3D<float> translate = { 0.0f, 0.0f, 0.0f };
    Vector3D<float> rotate = { 0.0f, 0.0f, 0.0f };
    Vector3D<float> scale = { 1.0f, 1.0f, 1.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransformGeometryModule)
};