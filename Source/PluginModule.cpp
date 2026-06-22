#include "PluginModule.h"
#include "Connection.h"
#include "AudioManager.h"
#include "Plugins/PluginManager.h"
#include "Plugins/SubprocessPluginScanner.h"
#include "Project/Project.h"
#include "Settings.h"


using namespace juce;

constexpr float defaultGainDb = 12.0f;

// Installed by MainComponent once the DockManager exists; null otherwise, in
// which case openPluginWindow() falls back to a floating DocumentWindow.
std::function<bool(PluginModule*)> PluginModule::activateDockedEditorHook;
std::function<bool(PluginModule*, juce::AudioProcessorEditor*, const juce::String&)> PluginModule::dockEditorHook;
std::function<void(PluginModule*)> PluginModule::closeDockedEditorHook;

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

	// Pre-allocate so addEvent never calls malloc on the hot path.
	midiBuffer.ensureSize(4096);

	createProperties();
    playHead = new OfflinePlayHead();
}

PluginModule::~PluginModule()
{
    // If this module's editor is shown in a dock panel, close it now — the
    // panel (and thus the editor) is owned by the DockManager, not by us, so
    // it would otherwise outlive `plugin` and dereference a dead processor.
    // Closing here destroys the editor while `plugin` is still alive.
    if (closeDockedEditorHook)
        closeDockedEditorHook(this);

    if (pluginValue)
        pluginValue->removeListener(pluginListener);
    delete pluginListener;
    delete pluginValue;
    delete playHead;
}

void PluginModule::setBuffersize(int buffersize)
{
    Module::setBuffersize(buffersize);
    if (this->buffersize == buffersize) return;
    this->buffersize = buffersize;

    // audioBuffer wird ohnehin pro Block in processInternal gegen
    // numSamples geprueft und ggf. via setSize neu dimensioniert — kein
    // separater Realloc hier noetig. bufferLeft/Right sind aktuell unused.

    // Plugin neu vorbereiten. releaseResources vor prepareToPlay ist JUCE-
    // Konvention und verhindert, dass strikte AU/VST3-Plugins die geaenderte
    // Block-Groesse ablehnen.
    const juce::ScopedLock sl(lock);
    if (plugin)
    {
        plugin->releaseResources();
        plugin->prepareToPlay(sampleRate, buffersize);
    }
}

void PluginModule::setSampleRate(float rate)
{
    Module::setSampleRate(rate);
    if ((float)sampleRate == rate) return;
    sampleRate = rate;

    const juce::ScopedLock sl(lock);
    if (plugin)
    {
        plugin->releaseResources();
        plugin->prepareToPlay(sampleRate, buffersize);
    }
}

void PluginModule::processBuffer(juce::AudioBuffer<float>& buf,
                                 juce::int64 samplePosition,
                                 bool isPlaying,
                                 bool isRecording) {
    const juce::ScopedLock sl(lock);
    if (plugin) {
        // Spiegelt den Playhead-Update aus process() — sonst sieht das Plugin
        // im Master-Pfad nur den Default-Stand (samplePos=0, playing=false).
        playHead->setPlaying(isPlaying);
        playHead->setRecording(isRecording);
        playHead->setPositionInfo(samplePosition, sampleRate, Project::getInstance()->getTempo());

        juce::MidiBuffer emptyMidi;
        plugin->processBlock(buf, emptyMidi);
    }
}

void PluginModule::openPluginWindow()
{
    if (!plugin) return;

    // Already shown somewhere — just bring it to the front. We check both the
    // floating window and the dock panel regardless of the current setting:
    // createEditorIfNeeded() hands back the *existing* editor, so re-wrapping an
    // editor that another owner already holds would double-free it. Focusing the
    // existing view sidesteps that and means toggling the setting mid-session is
    // always safe.
    if (win != nullptr)
    {
        win->setVisible(true);
        win->toFront(true);
        return;
    }

    if (activateDockedEditorHook && activateDockedEditorHook(this))
        return;

    // Not shown yet — create a fresh editor. Guard against plugins that return
    // nullptr.
    auto* editor = plugin->createEditorIfNeeded();
    if (editor == nullptr) return;

    // Docked mode (opt-in). The hook takes ownership of the editor; on success
    // we're done. If no host hook is wired (e.g. PLAYER mode) we fall through.
    if (Settings::getInstance()->getDockPluginWindows()
        && dockEditorHook
        && dockEditorHook(this, editor, pluginName))
        return;

    // Floating window (default / fallback). Pass a close callback so the window
    // destroys itself cleanly via win.reset().
    win = std::make_unique<PluginManager::PluginWindow>(
        pluginName, editor,
        [this]() { win.reset(); });

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

				// Sandbox-Validierung VOR der echten In-Process-Instanz:
				// Plugin in einem Kindprozess instanziieren + prepareToPlay
				// + ein paar processBlock. Crasht/haengt es dort, lehnen wir
				// es ab, statt Synthlab mitzureissen. Ergebnis wird gecached,
				// d.h. nur der erste Ladevorgang pro Plugin-Version testet.
				String valError;
				if (!SubprocessPluginScanner::validatePlugin(
						pd, device->getCurrentSampleRate(),
						device->getCurrentBufferSizeSamples(), valError))
				{
					AlertWindow::showMessageBoxAsync(
						AlertWindow::WarningIcon,
						"Plugin abgelehnt",
						"\"" + name + "\" wurde NICHT geladen.\n\n" + valError);
					return;
				}

				String error;
				auto instance = PluginManager::getInstance()->apfm
					->createPluginInstance(pd, device->getCurrentSampleRate(),
						device->getCurrentBufferSizeSamples(), error);

				if (!error.isEmpty() || instance == nullptr)
				{
					DBG("Plugin load error: " + error);
					return;
				}

				// Sidechain VOR prepareToPlay konfigurieren — setBusesLayout
				// aendert die Channel-Anzahl, ein bereits prepared Plugin
				// koennte das zurueckweisen. Setzt sidechainChannels (Member),
				// das beim spaeteren addSidechainPins() ausgelesen wird.
				trySetupSidechainBus(*instance);

				// Call prepareToPlay on the new instance before it becomes
				// visible to the audio thread. Use the device's actual values
				// (same ones used in createPluginInstance above) to avoid
				// crashes in strict AU plugins that validate these match.
				instance->prepareToPlay(device->getCurrentSampleRate(),
				                        device->getCurrentBufferSizeSamples());

				// Swap under the lock so the audio thread cannot be inside
				// processBlock while we destruct the old plugin instance.
				{
					const ScopedLock sl(lock);
					plugin.reset(instance.release());
				}

				// SC-Pins erst NACH dem Swap anhaengen — Module::addPin
				// pusht die Pins an's Ende von `pins` und ruft repaint, das
				// auf der Message-Thread-Seite passieren muss.
				if (sidechainChannels > 0)
					addSidechainPins();
			}
		}

		pluginName = name;
		pluginValue->setValue(name);

		if (!plugin)
			return;

		plugin->setPlayHead(playHead);

	}
}

void PluginModule::trySetupSidechainBus(juce::AudioPluginInstance& p)
{
	sidechainChannels = 0;

	// Plugin ohne 2. Input-Bus -> kein Sidechain, Defaults stehen lassen.
	if (p.getBusCount(true) < 2)
		return;

	if (p.getBus(true, 1) == nullptr)
		return;

	auto layout = p.getBusesLayout();
	if (layout.inputBuses.size() < 2)
		return;

	// Stereo zuerst — typischer Fall fuer Kompressoren / Gates / Vocoder.
	// Mono als Fallback fuer Plugins, deren SC-Bus nur Mono akzeptiert.
	layout.inputBuses.getReference(1) = juce::AudioChannelSet::stereo();
	if (p.setBusesLayout(layout))
	{
		sidechainChannels = 2;
		return;
	}

	layout.inputBuses.getReference(1) = juce::AudioChannelSet::mono();
	if (p.setBusesLayout(layout))
	{
		sidechainChannels = 1;
		return;
	}

	// Plugin will weder Stereo noch Mono Sidechain -> Layout vor unserer
	// Aenderung wiederherstellen (sonst bleibt es im "kaputten" Zustand).
	layout = p.getBusesLayout();
	(void) p.setBusesLayout(layout);
}

void PluginModule::addSidechainPins()
{
	if (sidechainChannels <= 0)
		return;

	// Erste SC-Pin immer. Bei Stereo-SC kommt SC R hinterher; Module::addPin
	// positioniert beide unter den existierenden Input-Pins (L/R/Gate).
	auto* l = new Pin(Pin::Type::AUDIO);
	l->direction = Pin::Direction::IN;
	l->setName(sidechainChannels == 1 ? "SC" : "SC L");
	l->listeners.push_back(this);
	addPin(Pin::Direction::IN, l);
	scLeftPinIdx = (int) pins.size() - 1;

	if (sidechainChannels == 2)
	{
		auto* r = new Pin(Pin::Type::AUDIO);
		r->direction = Pin::Direction::IN;
		r->setName("SC R");
		r->listeners.push_back(this);
		addPin(Pin::Direction::IN, r);
		scRightPinIdx = (int) pins.size() - 1;
	}

	// Modul-Hoehe um 20 px pro Pin nachschieben, sonst zeichnen die neuen
	// Pin-Punkte (y >= 70 bzw. 90) am unteren Rand des originalen 140-px-
	// Rahmens schon dicht — bei 90+10 ueber den Rand hinaus.
	setSize(getWidth(), getHeight() + sidechainChannels * 20);
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

    // Apply pending state changes from the message thread.
    // Doing this in the audio thread avoids racing plugin->reset()
    // against plugin->processBlock().
    if (needsReset.exchange(false))
        plugin->reset();
    
    int64_t newPos = nextSamplePosition.exchange(-1);
    if (newPos >= 0)
        currentSample = newPos;
    
	playHead->setPositionInfo(currentSample.load(), sampleRate, Project::getInstance()->getTempo());

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

	// Sidechain: pins[scLeftPinIdx] / pins[scRightPinIdx] in Buffer-Kanaele
	// 2 (+3) hineinkopieren. audioBuffer wurde oben auf max(numIn,numOut)
	// gesized; wenn der SC-Bus aktiv ist, ist numIn >= 3 (Mono-SC) bzw. >=4
	// (Stereo-SC). Unconnected SC-Pin -> Kanal bleibt 0 (clear() oben), das
	// Plugin sieht Stille, was korrektes "kein SC-Signal"-Verhalten ist.
	auto fillScChannel = [&](int pinIdx, int destChannel) {
		if (pinIdx < 0 || pinIdx >= (int) pins.size()) return;
		if (destChannel >= audioBuffer->getNumChannels()) return;
		auto& pin = *pins[pinIdx];
		if (pin.getConnections().empty()) return;
		auto* src = pin.getConnections()[0]->getAudioBuffer();
		if (src == nullptr) return;
		audioBuffer->copyFrom(destChannel, 0, *src, 0, 0, numSamples);
	};
	if (sidechainChannels >= 1) fillScChannel(scLeftPinIdx,  2);
	if (sidechainChannels >= 2) fillScChannel(scRightPinIdx, 3);

	// Snapshot MIDI under midiLock before acquiring the plugin lock — the two
	// locks must never be held simultaneously to prevent deadlock with
	// VPSAvenger2's long processBlock() call.
	// Use copy+clear (not swapWith) so midiBuffer keeps its pre-allocated storage
	// across every audio block; addEvent on the message thread never calls malloc.
	MidiBuffer midiSnapshot;
	if (!externalBuffer)
	{	
		const ScopedLock ml(midiLock);
		midiSnapshot = midiBuffer;  // deep-copy events into snapshot
		midiBuffer.clear();          // reset used-size; pre-allocated storage is kept
	}

	
	{
		const ScopedLock sl(lock);
		MidiBuffer* midiInBuffer = externalBuffer ? externalBuffer : &midiSnapshot;
		plugin->processBlock(*audioBuffer, *midiInBuffer);
	}
    
	if (numOut >= 2 && pins.size() >= 4)
	{
		auto* leftOut = pins[2]->getAudioBuffer();
		auto* rightOut = pins[3]->getAudioBuffer();

		if (leftOut && rightOut)
		{
			// force buffer size
		
			leftOut->setSize(1, numSamples, false, false, true);
			rightOut->setSize(1, numSamples, false, false, true);

			leftOut->copyFrom(0, 0, *audioBuffer, 0, 0, numSamples);
			rightOut->copyFrom(0, 0, *audioBuffer, 1, 0, numSamples);

			float gain = Decibels::decibelsToGain(bounceMode ? 6.0f : defaultGainDb);

			leftOut->applyGain(gain);
			rightOut->applyGain(gain);
		}
	}

	// Nur weiterzählen, wenn das Transport tatsächlich spielt. Sonst driftet
	// die ppqPosition während STOP und Plugins mit eigenem Sequenzer (Avenger,
	// Step-Sequencer, Arps) verlieren die Synchronisation.
	if (!bounceMode && playHead->isPlaying())
        currentSample.store(currentSample.load() + numSamples);
}


void PluginModule::eventReceived(Event* e)
{
	if (!e) return;

	// Copy the message then delete the event BEFORE acquiring midiLock.
	// If VPSAvenger2's processBlock holds the malloc lock (sample streaming),
	// the heap free would block; holding midiLock during that would deadlock
	// the audio thread's next processInternal swap.
	auto msg = e->getMessage();
	delete e;

	if (msg.isNoteOn() && msg.getVelocity() == 0)
		msg = MidiMessage::noteOff(msg.getChannel(), msg.getNoteNumber());

	msg.setTimeStamp(0);

	const ScopedLock ml(midiLock);
	midiBuffer.addEvent(msg, 0);
}

void PluginModule::addMidiMessage(const MidiMessage& msg)
{
	const ScopedLock ml(midiLock);
	midiBuffer.addEvent(msg, 0);
}

void PluginModule::resetPlugin()
{
	// Signal the audio thread to call reset() at the start of the next
	// processBlock. Calling reset() directly from the message thread would
	// race with the audio thread's processBlock (no lock around it anymore),
	// corrupting VPSAvenger2's internal state and causing CPU spin at 100%.
	needsReset.store(true);
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

void PluginModule::setCurrentProgram( int program)
{
	currentProgram = program;
	if (plugin && program >= 0 && program < plugin->getNumPrograms())
		plugin->setCurrentProgram(program);
}

void PluginModule::setPluginState(String state)
{
	auto mos = std::make_unique<MemoryOutputStream>();
	Base64::convertFromBase64(*mos, state);
	if (plugin) {
		// Plugin-Internas koennen bei manipuliertem/abgeschnittenem
		// State-Blob werfen oder ueber Bord gehen. Ein einzelner kaputter
		// Preset darf nicht das ganze Projekt-Load killen — wir loggen
		// und lassen das Plugin mit Default-Settings weiterlaufen.
		try {
			plugin->setStateInformation(mos->getData(), (int)mos->getDataSize());
		} catch (const std::exception& e) {
			juce::Logger::writeToLog("PluginModule::setPluginState: "
				+ juce::String(pluginName) + " threw during restore: "
				+ juce::String(e.what()));
		} catch (...) {
			juce::Logger::writeToLog("PluginModule::setPluginState: "
				+ juce::String(pluginName) + " threw unknown exception during restore.");
		}
	}
}

String PluginModule::getPluginState()
{
	if (plugin)
	{
		MemoryBlock mb;
		plugin->getStateInformation(mb);
		return Base64::toBase64(mb.getData(), mb.getSize());
	}
	return {};
}
