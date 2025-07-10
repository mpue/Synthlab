#include "PluginModule.h"
#include "Connection.h"
#include "AudioManager.h"
#include "Plugins/PluginManager.h"
#include "Project/Project.h"

using namespace juce;

PluginModule::PluginModule(double sampleRate, int buffersize)
    : sampleRate(sampleRate), buffersize(buffersize)
{
    setSize(120, 140);
    nameLabel->setJustificationType(Justification::left);
    nameLabel->setTopLeftPosition(18, 2);
    setName("Plugin");
    editable = false;
    prefab = true;

    bufferLeft = new float[buffersize];
    bufferRight = new float[buffersize];
    audioBuffer = new AudioBuffer<float>(2, buffersize);

    createProperties();
}

PluginModule::~PluginModule()
{
    delete[] bufferLeft;
    delete[] bufferRight;
    delete audioBuffer;
    delete pluginValue;
    delete pluginListener;
    delete win;
    delete plugin;
}

void PluginModule::openPluginWindow()
{
    if (!plugin) return;
    auto editor = plugin->createEditorIfNeeded();
    if (!win)
        win = new PluginManager::PluginWindow(pluginName, editor);
    else
        win->setVisible(true);
}

void PluginModule::selectPlugin(String name)
{
    if (name == pluginName) return;

    auto* manager = AudioManager::getInstance()->getDeviceManager();
    if (auto* device = manager->getCurrentAudioDevice())
    {
        if (!plugin)
        {
            PluginDescription pd;
            auto preset = File(File::getSpecialLocation(File::userHomeDirectory)
                .getChildFile(".Synthlab/plugins/" + name));
            auto xml = XmlDocument(preset).getDocumentElement();
            pd.loadFromXml(*xml);

            String error;
            plugin = PluginManager::getInstance()->apfm
                ->createPluginInstance(pd, device->getCurrentSampleRate(),
                    device->getCurrentBufferSizeSamples(), error)
                .release();
            plugin->prepareToPlay(sampleRate, buffersize);
        }

        pluginName = name;
        pluginValue->setValue(name);
    }
}

juce::String PluginModule::getPluginName()
{
    return pluginName;
}

void PluginModule::configurePins()
{
    const auto makeAudioIn = [this](String name) {
        auto* p = new Pin(Pin::Type::AUDIO);
        p->direction = Pin::Direction::IN;
        p->setName(name);
        p->listeners.push_back(this);
        addPin(Pin::Direction::IN, p);
        };

    const auto makeAudioOut = [this](String name) {
        auto* p = new Pin(Pin::Type::AUDIO);
        p->direction = Pin::Direction::OUT;
        p->setName(name);
        p->listeners.push_back(this);
        addPin(Pin::Direction::OUT, p);
        };

    makeAudioIn("L"); makeAudioIn("R");
    makeAudioOut("L"); makeAudioOut("R");

    auto* gate = new Pin(Pin::Type::EVENT);
    gate->direction = Pin::Direction::IN;
    gate->setName("G");
    gate->listeners.push_back(this);
    addPin(Pin::Direction::IN, gate);
}

void PluginModule::paint(Graphics& g)
{
    Module::paint(g);
    g.drawImageAt(ImageCache::getFromMemory(BinaryData::plugin_png, BinaryData::plugin_pngSize), 25, 40);
}

void PluginModule::process()
{
    const bool hasInputs =
        pins.size() >= 4 &&
        pins.at(0)->getConnections().size() > 0 &&
        pins.at(1)->getConnections().size() > 0 &&
        pins.at(0)->getConnections()[0]->getAudioBuffer() != nullptr &&
        pins.at(1)->getConnections()[0]->getAudioBuffer() != nullptr;


    if (plugin)
    {
        const ScopedLock sl(lock);

        // absichern: wie viele Eingänge/Outputs erwartet das Plugin?
        const int numIn = plugin->getTotalNumInputChannels();
        const int numOut = plugin->getTotalNumOutputChannels();

        // stelle sicher, dass audioBuffer korrekt dimensioniert ist
        if (audioBuffer->getNumChannels() != jmax(numIn, numOut) || audioBuffer->getNumSamples() != buffersize)
            audioBuffer->setSize(jmax(numIn, numOut), buffersize, false, false, true);

        if (hasInputs && numIn >= 2)
        {
            audioBuffer->copyFrom(0, 0, *pins.at(0)->getConnections()[0]->getAudioBuffer(), 0, 0, buffersize);
            audioBuffer->copyFrom(1, 0, *pins.at(1)->getConnections()[0]->getAudioBuffer(), 0, 0, buffersize);
        }
        else
        {
            audioBuffer->clear();
        }

        plugin->processBlock(*audioBuffer, midiBuffer);

        if (numOut >= 2 && pins.size() >= 4)
        {
            auto* leftOut = pins.at(2)->getAudioBuffer();
            auto* rightOut = pins.at(3)->getAudioBuffer();
            if (leftOut && rightOut)
            {
                leftOut->copyFrom(0, 0, *audioBuffer, 0, 0, buffersize);
                rightOut->copyFrom(0, 0, *audioBuffer, 1, 0, buffersize);

                float gain = Decibels::decibelsToGain(12.0f);
                leftOut->applyGain(gain);
                rightOut->applyGain(gain);
            }
        }
    }

    currentSample = (currentSample + 1) % buffersize;
    midiBuffer.clear();
}

void PluginModule::eventReceived(Event* e)
{
    const ScopedLock sl(lock);
    if (!e) return;

    auto msg = e->getMessage();

    // Handle NoteOn with velocity 0 as NoteOff
    if (msg.isNoteOn() && msg.getVelocity() == 0) {
        msg = MidiMessage::noteOff(msg.getChannel(), msg.getNoteNumber());
    }
    msg.setTimeStamp(currentSample);     
	
    midiBuffer.addEvent(msg, currentSample);
    delete e;
}

void PluginModule::createProperties()
{
    pluginValue = new Value();
    pluginListener = new PluginValueListener(*pluginValue, this);
}

Array<PropertyComponent*>& PluginModule::getProperties()
{
    properties.clear();
    Array<var> pluginList;
    for (auto& name : PluginManager::getInstance()->getPlugins())
        pluginList.add(name);

    pluginProp = new ChoicePropertyComponent(*pluginValue, "Plugin",
        PluginManager::getInstance()->getPlugins(), pluginList);
    properties.add(pluginProp);
    return properties;
}

int PluginModule::getCurrentProgram()
{
    if (plugin) currentProgram = plugin->getCurrentProgram();
    return currentProgram;
}

void PluginModule::setCurrentProgram(int program)
{
    currentProgram = program;
    if (plugin) plugin->setCurrentProgram(program);
}

void PluginModule::setPluginState(String state)
{
    auto* mos = new MemoryOutputStream();
    Base64::convertFromBase64(*mos, state);
    if (plugin)
        plugin->setStateInformation(mos->getData(), (int)mos->getDataSize());
    delete mos;
}

String PluginModule::getPluginState()
{
    if (plugin)
    {
        MemoryBlock mb(65536, true);
        plugin->getStateInformation(mb);
        return Base64::toBase64(mb.getData(), mb.getSize());
    }
    return {};
}
