#include "ComponentTreeDebugger.h"

ComponentTreeDebugger::ComponentTreeDebugger(juce::Component* rootComponent)
    : root(rootComponent)
{
    tree = std::make_unique<juce::TreeView>("ComponentTree");
    tree->setRootItemVisible(true);
    addAndMakeVisible(*tree);

    buildTree();
    startTimer(2000); // alle 2 Sekunden
}

ComponentTreeDebugger::~ComponentTreeDebugger() {}

void ComponentTreeDebugger::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey);
}

void ComponentTreeDebugger::resized()
{
    tree->setBounds(getLocalBounds());
}

void ComponentTreeDebugger::timerCallback()
{
    buildTree();
}

void ComponentTreeDebugger::buildTree()
{
	tree->removeAllChildren();
    if (root != nullptr && !root.wasObjectDeleted())
        tree->setRootItem(new ComponentItem(root.get()));
}

// ==============================

ComponentTreeDebugger::ComponentItem::ComponentItem(juce::Component* comp)
    : component(comp)
{
    setOpen(true);
}

bool ComponentTreeDebugger::ComponentItem::isValidComponent() const
{
    return component != nullptr && !component.wasObjectDeleted();
}

bool ComponentTreeDebugger::ComponentItem::mightContainSubItems()
{
    return isValidComponent() && component->getNumChildComponents() > 0;
}

void ComponentTreeDebugger::ComponentItem::paintItem(juce::Graphics& g, int width, int height)
{
    juce::String label = "<null>";

    if (isValidComponent())
    {
        label = component->getName();
        if (label.isEmpty())
            label = juce::String(typeid(*component.get()).name()).fromLastOccurrenceOf("::", false, true);
    }

    g.setColour(isSelected() ? juce::Colours::white : juce::Colours::lightgrey);
    g.drawText(label, 4, 0, width - 4, height, juce::Justification::centredLeft);
}

void ComponentTreeDebugger::ComponentItem::itemClicked(const juce::MouseEvent&)
{
    if (!isValidComponent())
        return;

    DBG("Clicked: " << component->getName());
    component->repaint();
}

void ComponentTreeDebugger::ComponentItem::itemOpennessChanged(bool isNowOpen)
{
    clearSubItems();

    if (isNowOpen && isValidComponent())
    {
        for (int i = 0; i < component->getNumChildComponents(); ++i)
        {
            auto* child = component->getChildComponent(i);
            if (child != nullptr)
                addSubItem(new ComponentItem(child));
        }
    }
}
