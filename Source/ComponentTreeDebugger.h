#pragma once

#include <JuceHeader.h>

class ComponentTreeDebugger : public juce::Component,
    private juce::Timer
{
public:
    ComponentTreeDebugger(juce::Component* rootComponent);
    ~ComponentTreeDebugger() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void buildTree();

    struct ComponentItem : public juce::TreeViewItem
    {
        ComponentItem(juce::Component* comp);
        bool mightContainSubItems() override;
        void paintItem(juce::Graphics&, int width, int height) override;
        void itemClicked(const juce::MouseEvent&) override;
        void itemOpennessChanged(bool isNowOpen) override;

        bool isValidComponent() const;

        juce::WeakReference<juce::Component> component;
    };

    std::unique_ptr<juce::TreeView> tree;
    juce::WeakReference<juce::Component> root;
};
