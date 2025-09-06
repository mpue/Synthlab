#include "PluginModule.h"
#include "Connection.h"
#include "AudioManager.h"
#include "Plugins/PluginManager.h"
#include "Project/Project.h"


using namespace juce;

constexpr float defaultGainDb = 12.0f;

PluginModule::PluginModule(double sampleRate, int buffersize)
	: sampleRate(sampleRate), buffersize(buffersize)
{
	setSize(120, 140);
	nameLabel = std::make_unique<Label>();
	nameLabel->setJustificationType(Justification::left);
	nameLabel->setTopLeftPosition(18, 2);
	addAndMakeVisible(*nameLabel);
	setName("Plugin");
	editable = false;
	prefab = true;

	bufferLeft = std::make_unique<float[]>(buffersize);
	bufferRight = std::make_unique<float[]>(buffersize);
	audioBuffer = std::make_unique<AudioBuffer<float>>(2, buffersize);

	createProperties();
}

PluginModule::~PluginModule() = default;

void PluginModule::openPluginWindow()
{
	if (!plugin) return;
	auto editor = plugin->createEditorIfNeeded();
	if (!win)
		win = std::make_unique<PluginManager::PluginWindow>(pluginName, editor);

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
			std::unique_ptr<XmlElement> xml(XmlDocument(preset).getDocumentElement());
			if (xml)
			{
				pd.loadFromXml(*xml);
				String error;
				auto instance = PluginManager::getInstance()->apfm
					->createPluginInstance(pd, device->getCurrentSampleRate(),
						device->getCurrentBufferSizeSamples(), error);

				if (!error.isEmpty() || instance == nullptr)
				{
					DBG("Plugin load error: " + error);
					return;
				}

				plugin.reset(instance.release());
				plugin->prepareToPlay(sampleRate, buffersize);				
				
			}
		}

		pluginName = name;
		pluginValue->setValue(name);

		playHead = OfflinePlayHead();

		if (!plugin)
			return;

		plugin->setPlayHead(&playHead);

	}
}

String PluginModule::getPluginName()
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

void PluginModule::process(MidiBuffer* externalBuffer, int numSamples)
{
	processInternal(externalBuffer, numSamples);
}

void PluginModule::process()
{
	if (!bounceMode)
		processInternal(nullptr, buffersize);
}

void PluginModule::process(MidiBuffer* externalBuffer)
{
	processInternal(externalBuffer, buffersize);
}

void PluginModule::processInternal(MidiBuffer* externalBuffer, int numSamples)
{
	if (!plugin)
		return;

	playHead.setPositionInfo(currentSample, sampleRate, Project::getInstance()->getTempo());	

	const int numIn = plugin->getTotalNumInputChannels();
	const int numOut = plugin->getTotalNumOutputChannels();

	if (audioBuffer->getNumChannels() != jmax(numIn, numOut) || audioBuffer->getNumSamples() != numSamples)
		audioBuffer->setSize(jmax(numIn, numOut), numSamples, false, false, true);

	audioBuffer->clear();

	const bool hasInputs =
		pins.size() >= 4 &&
		pins[0]->getConnections().size() > 0 &&
		pins[1]->getConnections().size() > 0 &&
		pins[0]->getConnections()[0]->getAudioBuffer() != nullptr &&
		pins[1]->getConnections()[0]->getAudioBuffer() != nullptr;

	if (hasInputs && numIn >= 2)
	{
		audioBuffer->copyFrom(0, 0, *pins[0]->getConnections()[0]->getAudioBuffer(), 0, 0, numSamples);
		audioBuffer->copyFrom(1, 0, *pins[1]->getConnections()[0]->getAudioBuffer(), 0, 0, numSamples);
	}

	{
		const ScopedLock sl(lock);
		MidiBuffer* midiInBuffer = externalBuffer ? externalBuffer : &midiBuffer;
		plugin->processBlock(*audioBuffer, *midiInBuffer);
	}

	if (!externalBuffer)
		midiBuffer.clear();

	if (numOut >= 2 && pins.size() >= 4)
	{
		auto* leftOut = pins[2]->getAudioBuffer();
		auto* rightOut = pins[3]->getAudioBuffer();

		if (leftOut && rightOut)
		{
			// ERZWINGE die Zielgröße des Outputs
			leftOut->setSize(1, numSamples, false, false, true);
			rightOut->setSize(1, numSamples, false, false, true);

			leftOut->copyFrom(0, 0, *audioBuffer, 0, 0, numSamples);
			rightOut->copyFrom(0, 0, *audioBuffer, 1, 0, numSamples);

			float gain = Decibels::decibelsToGain(bounceMode ? 6.0f : defaultGainDb);
			leftOut->applyGain(gain);
			rightOut->applyGain(gain);
		}
	}

	if (!bounceMode)
		currentSample = (currentSample + numSamples); //;% static_cast<int>(sampleRate);
}


void PluginModule::eventReceived(Event* e)
{
	const ScopedLock sl(lock);
	if (!e) return;

	auto msg = e->getMessage();

	if (msg.isNoteOn() && msg.getVelocity() == 0)
		msg = MidiMessage::noteOff(msg.getChannel(), msg.getNoteNumber());

	msg.setTimeStamp(0);
	midiBuffer.addEvent(msg, 0);
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
	auto mos = std::make_unique<MemoryOutputStream>();
	Base64::convertFromBase64(*mos, state);
	if (plugin)
		plugin->setStateInformation(mos->getData(), (int)mos->getDataSize());
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
