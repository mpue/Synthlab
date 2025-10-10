/*
  ==============================================================================
    FeedbackModule.h
    Created: 10 Oct 2025
    Author:  mpue
  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"

using namespace juce;

//==============================================================================
/*
 */
class FeedbackModule : public Module
{
public:
    FeedbackModule();
    ~FeedbackModule();

    void paint(juce::Graphics& g) override;
    virtual void process() override;
    virtual void configurePins() override;

    virtual juce::String getCategory() override {
        return "Math";
    }

    virtual juce::String getDescription() override {
        return "Allows feedback loop by storing previous value";
    }

    virtual juce::Array<juce::PropertyComponent*>& getProperties() override;
    virtual void createProperties() override;

private:

    float fbValue = 0.0f;
    float initValue = 0.0f;
    float damping = 1.0f;  // 0.0 - 1.0, where 1.0 = no damping

    juce::Value* initialValue = nullptr;
    juce::Value* dampingValue = nullptr;

    juce::PropertyComponent* initialProp = nullptr;
    juce::PropertyComponent* dampProp = nullptr;

    struct InitialValueListener : juce::Value::Listener
    {
        InitialValueListener(juce::Value& v, FeedbackModule* module)
            : module(module), value(v) {
            value.addListener(this);
        }
        ~InitialValueListener() {}

        void valueChanged(juce::Value& value) override {
            module->setInitialValue(value.getValue());
        }
        FeedbackModule* module;
        juce::Value value;
    };

    struct DampingListener : juce::Value::Listener
    {
        DampingListener(juce::Value& v, FeedbackModule* module)
            : module(module), value(v) {
            value.addListener(this);
        }
        ~DampingListener() {}

        void valueChanged(juce::Value& value) override {
            module->setDamping(value.getValue());
        }
        FeedbackModule* module;
        juce::Value value;
    };

    InitialValueListener* initialValueListener;
    DampingListener* dampingListener;

    void setInitialValue(float value) {
        initialValue->setValue(value);
    }

    void setDamping(float value) {
        damping = juce::jlimit(0.0f, 1.0f, (float)value);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FeedbackModule)
};