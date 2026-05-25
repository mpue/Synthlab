#include "ControlSurfacePanel.h"

#include "SynthEditor.h"
#include "PrefabFactory.h"
#include "AudioManager.h"
#include "Knob.h"
#include "SliderModule.h"

ControlSurfacePanel::ControlSurfacePanel()
{
    refreshButton = std::make_unique<juce::TextButton>("Refresh");
    refreshButton->addListener(this);
    addAndMakeVisible(*refreshButton);
}

ControlSurfacePanel::~ControlSurfacePanel()
{
    if (editor != nullptr)
        editor->removeEditorListener(this);
    clearShadow();
}

void ControlSurfacePanel::setEditor (SynthEditor* e)
{
    editor = e;
    if (editor != nullptr)
        editor->addEditorListener(this);
    rebuild();
}

void ControlSurfacePanel::beforeCleanUp()
{
    // Source modules are about to be destroyed. Drop all shadow widgets NOW
    // while their bound source-ValueSources are still alive — otherwise the
    // shadow Slider destructors would dereference freed ValueSource pointers
    // when Value::referTo tears down the listener registration.
    clearShadow();
}

void ControlSurfacePanel::buttonClicked (juce::Button* b)
{
    if (b == refreshButton.get())
        rebuild();
}

void ControlSurfacePanel::clearShadow()
{
    // Order matters: groups own the GroupComponents which contain the shadow
    // module Components as visual children. Detach the shadows first so the
    // GroupComponent destructor doesn't try to delete components we own via
    // unique_ptr.
    for (auto& e : entries) {
        if (e.shadow != nullptr) {
            if (auto* p = e.shadow->getParentComponent())
                p->removeChildComponent(e.shadow.get());
        }
    }
    entries.clear();
    groups.clear();
}

// Pull state from a freshly-loaded source onto the shadow. The shadow was just
// instantiated by PrefabFactory with default config; without this, every
// shadow Knob would read 0..127 / step 1 / value 0 regardless of the source.
void ControlSurfacePanel::copyConfig (Module* source, Module* shadow)
{
    if (source == nullptr || shadow == nullptr) return;

    shadow->setName(source->getName());
    // Bewusst KEIN setSize vom Source: der Source ist evtl. im ALL-Layer
    // (kompakt 90x30 fuer Knob) — das Panel will die volle GUI-Darstellung,
    // die Knob/SliderModule per default schon im Ctor setzen.

    if (auto* k = dynamic_cast<Knob*>(source)) {
        if (auto* ks = dynamic_cast<Knob*>(shadow)) {
            ks->setMinimum(k->getMinimum());
            ks->setMaximum(k->getMaximum());
            ks->setStepSize(k->getStepsize());
            ks->setValue(k->getValue());
        }
    }
    if (auto* s = dynamic_cast<SliderModule*>(source)) {
        if (auto* ss = dynamic_cast<SliderModule*>(shadow)) {
            ss->setMinimum(s->getMinimum());
            ss->setMaximum(s->getMaximum());
            ss->setStepSize(s->getStepsize());
            ss->setValue(s->getValue());
        }
    }
}

// Two-way live binding via juce::Value::referTo — both Sliders end up sharing
// the same Value object, so dragging either drives the other and the source's
// internal sliderValueChanged listeners fire as if the user had touched the
// original control directly.
void ControlSurfacePanel::bindSliderPair (Module* source, Module* shadow)
{
    juce::Slider* srcSlider = nullptr;
    juce::Slider* shdSlider = nullptr;

    if (auto* k = dynamic_cast<Knob*>(source))         srcSlider = k->getSlider();
    else if (auto* s = dynamic_cast<SliderModule*>(source)) srcSlider = s->getSlider();

    if (auto* k = dynamic_cast<Knob*>(shadow))         shdSlider = k->getSlider();
    else if (auto* s = dynamic_cast<SliderModule*>(shadow)) shdSlider = s->getSlider();

    if (srcSlider == nullptr || shdSlider == nullptr) return;

    shdSlider->setRange(srcSlider->getMinimum(), srcSlider->getMaximum(), srcSlider->getInterval());
    shdSlider->getValueObject().referTo(srcSlider->getValueObject());
}

void ControlSurfacePanel::rebuild()
{
    clearShadow();

    if (editor == nullptr || editor->getModule() == nullptr) {
        repaint();
        return;
    }

    auto* device = AudioManager::getInstance()->getDeviceManager()->getCurrentAudioDevice();
    const float sampleRate = device != nullptr ? (float) device->getCurrentSampleRate() : 44100.0f;
    const int   bufferSize = device != nullptr ? device->getCurrentBufferSizeSamples() : 512;

    // 1. Bucket source controls by their immediate parent (mirrors the
    //    grouping that collectRootLevelControls uses inside the main editor —
    //    keep the visual structure identical so the user recognises the
    //    layout).
    std::map<Module*, std::vector<Module*>> bucket;
    std::function<void(Module*)> walk = [&](Module* c) {
        for (auto* m : *c->getModules()) {
            if (m == nullptr) continue;
            if (m->getLayer() == Module::Layer::GUI
                && m->getDisplayAtRootLevel()
                && c != editor->getModule()) {
                bucket[c].push_back(m);
            }
            if (! m->getModules()->empty())
                walk(m);
        }
    };
    walk(editor->getModule());

    // 2. Stack groups vertically. Each group owns one shadow per source
    //    control; the source stays where it is in the main editor.
    const int padding      = 10;
    const int titleHeight  = 25;
    const int controlGap   = 5;
    int groupY = kHeaderHeight + padding;

    for (auto& entry : bucket) {
        Module* source = entry.first;
        auto& controls = entry.second;

        auto* group = new juce::GroupComponent(source->getName(), source->getName());
        group->setColour(juce::GroupComponent::outlineColourId, juce::Colours::grey);
        group->setColour(juce::GroupComponent::textColourId,    juce::Colours::white);

        int autoX = padding;
        int contentRight  = padding;
        int contentBottom = titleHeight;

        for (auto* m : controls) {
            Module* shadow = PrefabFactory::getInstance()->getPrefab(m->getId(), sampleRate, bufferSize);
            if (shadow == nullptr) continue;

            copyConfig(m, shadow);
            // Shadow ist immer "Play-Mode": Slider akzeptiert Eingaben, auch
            // wenn der Haupteditor im Edit-Mode (locked=false) ist. Die Panel-
            // Halbsemantik ist "live spielen", nicht "bearbeiten".
            shadow->setLocked(true);
            bindSliderPair(m, shadow);

            // Layout inside the group: row of controls, wrap not supported in
            // v1 — fine for the typical handful of surfaced knobs per source.
            shadow->setTopLeftPosition(autoX, titleHeight);
            autoX += shadow->getWidth() + controlGap;

            contentRight  = std::max(contentRight,  autoX);
            contentBottom = std::max(contentBottom, titleHeight + shadow->getHeight());

            group->addAndMakeVisible(shadow);

            ShadowEntry e;
            e.source = m;
            e.shadow.reset(shadow);
            entries.push_back(std::move(e));
        }

        const int groupW = std::max(contentRight + padding, 150);
        const int groupH = contentBottom + padding;
        group->setBounds(padding, groupY, groupW, groupH);
        groupY += groupH + 10;

        addAndMakeVisible(group);
        groups.add(group);
    }

    resized();
    repaint();
}

void ControlSurfacePanel::paint (juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1e1e1e));

    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(14.0f, juce::Font::bold));
    g.drawText("Controls", 10, 0, getWidth() - 20, kHeaderHeight,
               juce::Justification::centredLeft, true);
}

void ControlSurfacePanel::resized()
{
    if (refreshButton != nullptr)
        refreshButton->setBounds(getWidth() - 90, 4, 80, kHeaderHeight - 8);
}
