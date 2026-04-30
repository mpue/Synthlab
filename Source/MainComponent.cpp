/*
  ==============================================================================

	This file was auto-generated!

  ==============================================================================
*/


#include <vector>
#include "Plugins/PluginManager.h"
#include "PluginModule.h"

#include "AudioManager.h"
#include "MainComponent.h"
#include "SynthEditor.h"
#include "PropertyView.h"
#include "MainTabbedComponent.h"
#include "MidiGate.h"
#include "MidiNote.h"
#include "PrefabFactory.h"
#include "Project/Project.h"
#include "Knob.h"
#include "Mixer.h"
#include "Actions/RemoveSelectedAction.h"
#include "Actions/AddModuleAction.h"
#include "ModuleBrowser.h"
#include "PitchBendModule.h"
#include "ModuleUtils.h"
#include "MidiClock.h"
#include "MidiRecorderModule.h"
#include "TrackUtil.h"
#include "Tracks/BounceThread.h"
#include "BounceDialog.h"
#include "FadeDialog.h"
#include "GainDialog.h"
#include "SilenceDialog.h"
#include "TrimDialog.h"
#include "ChatGPTInterface.h"
#include "AudioEngine/BPMDetector.h"
#include "TrackIn.h"
#include "Module.h"
#include "AudioUtils.h"
#include "Actions/NormalizeRegionAction.h"
#include "Actions/FadeRegionAction.h"
#include "Actions/TrackDeleteAction.h"

using juce::String;
using juce::File;
using juce::MenuBarComponent;
using juce::MidiInput;
using juce::Label;
using juce::Slider;
using juce::ScopedPointer;
using juce::XmlDocument;
using juce::XmlElement;
using juce::Toolbar;
using juce::Colour;
using juce::MouseEvent;
using juce::ChoicePropertyComponent;
using juce::MemoryBlock;
using juce::var;
using juce::Image;
using juce::Logger;
using juce::AudioSourceChannelInfo;
using juce::DragAndDropTarget;
using juce::Time;
using juce::Graphics;
using juce::PopupMenu;
using juce::ResizableWindow;
using juce::JUCEApplication;
using juce::StringArray;
using juce::AudioDeviceSelectorComponent;
using juce::DialogWindow;
using juce::LookAndFeel;
using juce::AudioDeviceManager;
using juce::ModalComponentManager;
using juce::ModalCallbackFunction;
using juce::KeyPress;
using juce::Button;
using juce::ToolbarButton;

//==============================================================================
MainComponent::MainComponent() : resizerBar(&stretchableManager, 1, true)
{

	// Make sure you set the size of the component after
	// you add any child components.
	setSize(1280, 800);

	// Step 1: always open the system default device first – this guarantees that
	// the AudioSourcePlayer is properly connected and delivering callbacks.
	setAudioChannels(2, 2);

	// Step 2: if a saved config exists, switch to it after a short delay.
	// This replicates what the user has to do manually ("switch to internal,
	// then switch to Tascam") – some USB interfaces only work when the device
	// manager performs an explicit device switch rather than opening them cold.
	auto savedAudioConfig = loadAudioConfig();
	if (savedAudioConfig != nullptr)
	{
		juce::String savedXml = savedAudioConfig->toString();
		juce::Timer::callAfterDelay(1500, [this, savedXml]()
		{
			auto xml = juce::XmlDocument::parse(savedXml);
			deviceManager.initialise(2, 2, xml.get(), true);
			deviceManager.restartLastAudioDevice();
			AudioManager::getInstance()->setupChannels();
			if (midiProcessor != nullptr)
				midiProcessor->refreshMidiInputs();
		});
	}

	createConfig();

	commandManager.reset(new juce::ApplicationCommandManager());
	
	addKeyListener(commandManager->getKeyMappings());

	AudioManager::getInstance()->setDeviceManager(&deviceManager);
	Project::getInstance()->setAudioDeviceManager(&deviceManager);
	Project::getInstance()->setCommandManager(commandManager.get());

	if (Project::getInstance()->getAppMode() == Project::AppMode::STUDIO) {

#if defined(JUCE_PLUGINHOST_AU) || defined(JUCE_PLUGINHOST_VST)
		PluginManager::getInstance();
#endif

		createToolbar();
		createCPUMeter();
		createMenu();
		createKeyMap();
		createStudioLayout();

		// a global sampler object which allows us to play audio at any place like for preview for example
		defaultSampler = new Sampler(sampleRate, buffersize);
		Project::getInstance()->setDefaultSampler(defaultSampler);
	}
	else if (Project::getInstance()->getAppMode() == Project::AppMode::PLAYER) {
		createPlayerLayout();
	}

	Project::getInstance()->setMain(this);

	startTimer(40);

	midiProcessor = new MidiProcessor(editor, &deviceManager);
	midiProcessor->enableAllMidiInputs();
	Project::getInstance()->setMidiProcessor(midiProcessor);

	// addMouseListener(this, true);
	editor->addKeyListener(commandManager->getKeyMappings());
	addKeyListener(commandManager->getKeyMappings());
	setApplicationCommandManagerToWatch(commandManager.get());
	addKeyListener(this);
	editor->setMouseClickGrabsKeyboardFocus(true);
	editor->setWantsKeyboardFocus(true);
	resized();
	repaint();
	initialized = true;
	setRunning(true);

	if (Project::getInstance()->getAppMode() == Project::AppMode::PLAYER) {
		editor->loadFromString(String(BinaryData::_3oscV2_slb));
		editor->setCurrentLayer(Module::Layer::GUI);
		editor->setLocked(true);
	}
	commandManager->registerAllCommandsForTarget(this);
};

MainComponent::~MainComponent()
{
	running = false;

	shutdownAudio();

	if (Project::getInstance()->getAppMode() == Project::AppMode::STUDIO) {
		loadSlider->setLookAndFeel(nullptr);

#if JUCE_MAC
		MenuBarModel::setMacMainMenu(nullptr);
#endif
	}

	editor->removeAllChangeListeners();

	midiProcessor->disableAllMidiInputs();

	if (moduleBrowser != nullptr) {
		if (moduleBrowser->isVisible()) {
			moduleBrowser->setVisible(false);
		}
		moduleBrowser = nullptr;
	}

	if (Project::getInstance()->getAppMode() == Project::AppMode::STUDIO) {
		tab = nullptr;

		propertyView = nullptr;
		delete lockButton;
		menu = nullptr;
		toolbar = nullptr;
		toolbarFactory = nullptr;
		editorView = nullptr;
#if defined(JUCE_PLUGINHOST_AU)
		delete pluginMenu;
#endif
		delete cpuLoadLabel;
		delete loadSlider;

	}
	else if (Project::getInstance()->getAppMode() == Project::AppMode::PLAYER) {
		delete editor;
		tab = nullptr;
		propertyView = nullptr;
		toolbarFactory = nullptr;
		editorView = nullptr;
		mixerPanel = nullptr;
		delete lockButton;
	}

	if (defaultSampler != nullptr) {
		delete defaultSampler;
	}

	stopTimer();

	delete midiProcessor;
	chat = nullptr;

	PrefabFactory::destroy();
	Project::getInstance()->removeAllChangeListeners();
	Project::destroy();

	// This shuts down the audio device and clears the audio source.
	rotatingCube = nullptr;
}

void MainComponent::createMenu() {
	this->menu = std::make_unique<MenuBarComponent>();

#if JUCE_MAC
	menu->setModel(nullptr);
	MenuBarModel::setMacMainMenu(this);
#else
	menu->setModel(this);
	menu->setBounds(0, 0, 1000, 25);
	addAndMakeVisible(menu.get());
#endif
}

std::unique_ptr<juce::XmlElement> MainComponent::loadAudioConfig()
{
	String userHome = File::getSpecialLocation(File::userHomeDirectory).getFullPathName();
	File configFile = File(userHome + "/.Synthlab/config.xml");

	if (configFile.exists())
		return XmlDocument(configFile).getDocumentElement();

	return nullptr;
}

void MainComponent::createConfig() {
	String userHome = File::getSpecialLocation(File::userHomeDirectory).getFullPathName();

	File appDir = File(userHome + "/.Synthlab");

	if (!appDir.exists()) {
		appDir.createDirectory();
	}
	// Audio device is already initialised via setAudioChannels() in the constructor.
}

void MainComponent::createToolbar() {
	toolbar = std::make_unique<Toolbar>();

#if JUCE_MAC

#else
	toolbar->setBounds(0, 25, getWidth(), 50);
#endif

	addAndMakeVisible(toolbar.get());

	toolbarFactory = std::make_unique<DefaultToolbarItemFactory>();

	for (int i = 0; i < toolbarFactory->numItems(); i++) {
		toolbar->addItem(*toolbarFactory, i + 1);
		toolbar->getItemComponent(i)->addListener(this);

		if (layerCombobox == nullptr) {
			layerCombobox = dynamic_cast<ToolbarComboBox*>(toolbar->getItemComponent(i));
		}
		if (layerCombobox != nullptr) {

			layerCombobox->getComboBox().addListener(this);
		}
	}

	lockButton = new ImageButton("Lock");

	Image normalImage = juce::ImageCache::getFromMemory(BinaryData::unlock_png, BinaryData::unlock_pngSize);
	Image downImage = juce::ImageCache::getFromMemory(BinaryData::lock_png, BinaryData::lock_pngSize);

	lockButton->setImages(false, true, true, normalImage, 1.0f, juce::Colours::white, normalImage, 1.0, juce::Colours::white, downImage, 1.0f, juce::Colours::white);
	lockButton->setClickingTogglesState(true);
	lockButton->setSize(24, 24);
	lockButton->setTopLeftPosition(1028 + 480 + 6, 13);
	toolbar->addAndMakeVisible(lockButton);

	lockButton->addListener(this);

}

void MainComponent::createCPUMeter() {
	cpuLoadLabel = new Label("0%");
	cpuLoadLabel->setText("0%", juce::NotificationType::dontSendNotification);
	toolbar->addAndMakeVisible(cpuLoadLabel);

	loadSlider = new Slider();

	loadSlider->setRange(0, 100, 1);
	loadSlider->setSliderStyle(Slider::LinearBar);
	loadSlider->setTextBoxStyle(Slider::NoTextBox, false, 80, 20);
	loadSlider->setColour(Slider::backgroundColourId, Colour(0xff313131));
	loadSlider->setColour(Slider::thumbColourId, Colours::chartreuse);
	loadSlider->setColour(Slider::trackColourId, Colour(0xff434242));
	loadSlider->setColour(Slider::rotarySliderOutlineColourId, Colour(0x66ffffff));
	loadSlider->setColour(Slider::textBoxBackgroundColourId, Colour(0x00ffffff));
	loadSlider->setLookAndFeel(Project::getInstance()->getLookAndFeel());

	addAndMakeVisible(loadSlider);
}

void MainComponent::createStudioLayout() {
	propertyView = std::make_unique<PropertyView>();
	parser = new CommandParser();
	transportDisplay = std::make_unique<TransportDisplayComponent>();
	transportDisplay->setTransportState(0, sampleRate, 120.0, 4, 4);
	transportDisplay->setMetronomeState(false);
	transportDisplay->setSize(480, 48);
#if JUCE_MAC
	transportDisplay->setTopLeftPosition(1028, 1);
#else
	transportDisplay->setTopLeftPosition(1028, 26);
#endif
	addAndMakeVisible(transportDisplay.get());

	metronome = std::make_unique<Metronome>(44100, 128);
	metronome->setBPM(Project::getInstance()->getTempo()); // oder Wert aus TransportDisplay

	editorView = std::make_unique<EditorComponent>(sampleRate, buffersize, propertyView.get(), *parser, transportDisplay.get());
	editor = editorView.get()->getEditor();
	mixerPanel = editorView->getMixerPanel();
	mixerPanel->setSynthEditor(editor);

	if (navigator == nullptr) {
		navigator = editorView->getNavigator();
	}

	navigator->addChangeListener(this);
	getTopLevelComponent()->addKeyListener(navigator);

	parser->registerCommand("help", [this](const std::vector<std::string>&) -> std::string {
		return parser->displayHelp();
		}, "Show help.");

	parser->registerCommand("scan", [](const std::vector<std::string>&) -> std::string {
		PluginManager::getInstance()->scanPlugins();
		return "Plugin scan started.";
		}, "Start plugin scan.");

	parser->registerCommand("add", [this](const std::vector<std::string>& args) -> std::string {
		if (args.empty()) {
			return "Error: No arguments provided.";
		}

		Point<int> pos = Point<int>(100, 100);

		if (args[0] == "instrument") {
			if (args.size() < 2) return "Error: Missing instrument name.";
			TrackUtil::createVirtualInstrumentTrack(args.at(1), pos, navigator, editor);
			return "Instrument track added.";
		}
		if (args[0] == "midi") {
			TrackUtil::createMIDITrack(editor, navigator, editor->getModule());
			return "MIDI track added.";
		}
		if (args[0] == "audio") {
			TrackUtil::createAudioTrack(editor, navigator, editor->getModule());
			return "Audio track added.";
		}
		if (args[0] == "midiregion") {
			if (args.size() < 3) return "Error: Missing arguments for MIDI region.";
			navigator->getCurrentTrack()->addMidiRegion(Project::getInstance()->getSampleRate(), juce::String(args[1]).getIntValue(), juce::String(args[2]).getIntValue());
			return "MIDI region added.";
		}

		return "Error: Unknown argument.";
		}, "Add tracks and regions\n.Available options are midi, audio and midiregion.\nAdding a midi region requires 2 arguments samplepos and position in samples.");

	parser->registerCommand("list", [this](const std::vector<std::string>& args) -> std::string {
		if (args.empty()) {
			return "Error: No arguments provided.";
		}

		if (args[0] == "regions") {
			std::stringstream ss;

			if (navigator->getCurrentTrack() == nullptr) {
				return "Error: No track selected.";
			}

			for (auto region : navigator->getCurrentTrack()->getRegions()) {
				ss << "Region " << region->getSampleOffset() << " - " << region->getNumSamples() << "\n";
			}

			return ss.str();
		}
		if (args[0] == "messages") {

			if (navigator->getCurrentTrack() == nullptr) {
				return "Error: No track selected.";
			}
			if (navigator->getCurrentTrack()->getCurrentRegion() == nullptr) {
				return "Error: No region found.";
			}

			MidiRegion* r = dynamic_cast<MidiRegion*>(navigator->getCurrentTrack()->getCurrentRegion());

			std::stringstream ss;

			for (auto message : r->getMessages()) {
				ss << "Message at " << String(message->getTimeStamp()) << " - " << message->getDescription() << std::endl;
			}

			return ss.str();
		}
		return "Error: Unknown argument.";
		}, "List regions of current track.");

	parser->registerCommand("set", [this](const std::vector<std::string>& args) -> std::string {
		if (args.size() < 2) return "Error: Missing arguments.";

		if (args[0] == "track") {
			if (args[1] == "volume")
			{
				float value = juce::String(args[2]).getFloatValue();
				mixerPanel->getChannels().at(0)->setVolume(value);
			}
			else if (args[1] == "pan") {
				float value = juce::String(args[2]).getFloatValue();
				Mixer::getInstance()->getChannel(Mixer::Channel::Type::OUT, 0)->pan = value;
			}
			else if (args[1] == "mute") {
				bool value = juce::String(args[2]).getIntValue() == 1 ? true : false;
				Mixer::getInstance()->getChannel(Mixer::Channel::Type::OUT, 0)->mute = value;
			}
			else if (args[1] == "solo") {
				bool value = juce::String(args[2]).getIntValue() == 1 ? true : false;
				Mixer::getInstance()->getChannel(Mixer::Channel::Type::OUT, 0)->solo = value;
			}
			else {
				return "Error: Unknown argument.";
			}
		}
		else if (args[0] == "region")
		{
			if (args[1] == "offset") {

				Region* r = navigator->getCurrentTrack()->getRegionAt(juce::String(args[2]).getIntValue());
				if (r == nullptr) {
					return "Error: Region not found.";
				}
				r->setSampleOffset(juce::String(args[1]).getIntValue(), false, false);
				return "Region updated.";

			}
		}
		}, "set [sample in region][samplepos]");

	parser->registerCommand("get", [this](const std::vector<std::string>& args) -> std::string {
		if (args.empty()) {
			return "Error: No arguments provided.";
		}

		std::stringstream ss;

		if (args[0] == "region") {
			if (args.size() < 2) return "Error: Missing region index.";

			Region* r = navigator->getCurrentTrack()->getRegionAt(juce::String(args[1]).getIntValue());
			if (r != nullptr) {
				ss << "Region " << r->getSampleOffset() << " - " << r->getNumSamples();
			}
			else {
				return "Error: Region not found.";
			}
		}
		else if (args[0] == "currentregion") {
			Region* r = navigator->getCurrentTrack()->getCurrentRegion();
			if (r != nullptr) {
				ss << "Region " << r->getSampleOffset() << " - " << r->getNumSamples();
			}
			else {
				return "Error: Current region not found.";
			}
		}
		else {
			return "Error: Unknown argument.";
		}

		return ss.str();
		}, "Gets a value.");

	parser->registerCommand("export", [this](const std::vector<std::string>& args) -> std::string {
		if (args.empty()) {
			return "Error: No arguments provided.";
		}
		String size = args[1];
		setRunning(false);
		setBouncing(true);

		// Alle Plugin-Module für Bounce-Modus vorbereiten
		for (int i = 0; i < editor->getModule()->getModules()->size(); i++) {
			if (dynamic_cast<TrackIn*>(editor->getModule()->getModules()->at(i)) != nullptr) {
				TrackIn* t = dynamic_cast<TrackIn*>(editor->getModule()->getModules()->at(i));
				t->setBouncing(true);
			}
		}

		startBounce(File(args[0]), size.getFloatValue());
		return "Export finished.";
		}, "Export track as audio.");

	parser->registerCommand("stop", [&](const std::vector<std::string>&) -> std::string {
		setRunning(false);
		return "Engine stopped.";
		}, "Stop the audio engine.");

	parser->registerCommand("run", [&](const std::vector<std::string>&) -> std::string {
		setRunning(true);
		return "Engine started.";
		}, "Start the audio engine.");


	chat = std::make_unique<ChatGPTInterface>(loadClaudeApiKey());

	auto claudePromptHandler =
		[this](const std::vector<std::string>& args) -> std::string
		{
			if (args.empty())
				return "Usage: claude <frage>";

			std::string prompt;
			for (const auto& w : args)
				prompt += w + " ";

			std::string response = chat->sendPrompt(prompt);
			juce::String parsed = tryExecuteChatInstruction(response);

			return parsed.toStdString();
		};

	parser->registerCommand("claude", claudePromptHandler, "Frage an Claude senden");
	// Alias — ChatGPTInterface ist der historische Name; Console-User tippen
	// vermutlich "chatgpt" aus Gewohnheit.
	parser->registerCommand("chatgpt", claudePromptHandler, "Frage an Claude senden (alias)");

#if JUCE_MAC

#else
	propertyView->setTopLeftPosition(0, 75);
#endif
	editor->addEditorListener(propertyView.get());
	propertyView->getBrowser()->addChangeListener(editor);
	// Hand the editor to PropertyView so its Instruments tab can spawn an
	// Instrument track on double-click without a plugin browser dialog.
	propertyView->setEditor(editor);

	Project::getInstance()->setPropertyView(propertyView.get());

	addAndMakeVisible(propertyView.get());
	addAndMakeVisible(resizerBar);
	addAndMakeVisible(editorView.get());
	editor->setTab(editorView->getEditorTab());
	editorView->getEditorTab()->addChangeListener(this);
	editor->addEditorListener(this);
	addMouseListener(propertyView->getBrowser(), false);

	commandManager->registerAllCommandsForTarget(editor);
	commandManager->setFirstCommandTarget(editor);

	// we have to set up our StretchableLayoutManager so it know the limits and preferred sizes of it's contents
	stretchableManager.setItemLayout(0,            // for the properties
		-0.1, -0.9,   // must be between 50 pixels and 90% of the available space
		-0.2);        // and its preferred size is 30% of the total available space

	stretchableManager.setItemLayout(1,            // for the resize bar
		5, 5, 5);     // hard limit to 5 pixels

	stretchableManager.setItemLayout(2,            // for the main editor
		-0.1, -0.9,   // size must be between 50 pixels and 90% of the available space
		-0.8);

#if defined(JUCE_PLUGINHOST_AU) || defined(JUCE_PLUGINHOST_VST)
	pluginMenu = PluginManager::getInstance()->buildPluginMenu();
#endif
	editor->updateProject(File());
	levelMeter = std::make_unique<AudioLevelMeter>();
	Project::getInstance()->getSupplemental()->addTab("Level Meter", Colours::darkgrey, levelMeter.get(), false);
	spectrumAnalyzer = std::make_unique<SpectrumAnalyzer>();
	spectrumAnalyzer->setNumberOfBars(128);
	Project::getInstance()->getSupplemental()->addTab("Spektrum", Colours::darkgrey, spectrumAnalyzer.get(), false);
}

void MainComponent::createPlayerLayout() {
	editor = new SynthEditor(sampleRate, buffersize);
	mixerPanel = std::make_shared<MixerPanel>();
	editor->setMixer(mixerPanel);
	mixerPanel->setSynthEditor(editor);
	addAndMakeVisible(editor);
	addChildComponent(mixerPanel.get());
}

void MainComponent::timerCallback() {

	if (mixerPanel != nullptr) {
		for (auto* panel : mixerPanel->getChannels())
			panel->updateMeter();
	}

	if (Project::getInstance()->getAppMode() == Project::AppMode::STUDIO) {

		currentMeasure = (currentMeasure + 1) % 10;

		loads[currentMeasure] = cpuLoad;

		if (currentMeasure == 0) {
			for (int i = 0; i < 10; i++) {
				cpuLoad += loads[i];
			}
			cpuLoad /= 10;

			if (cpuLoad < 0) {
				cpuLoad = 0;
			}
			if (cpuLoad == NAN) {
				cpuLoad = 0;
			}

			loadSlider->setValue(cpuLoad);
			cpuLoadLabel->setText(String(cpuLoad) + "%", juce::NotificationType::dontSendNotification);
		}
	}

}

//==============================================================================
void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
	Logger::writeToLog("prepare to play with sample rate " + String(sampleRate) + " kHz and buffer size of " + String(buffersize) + " bytes.");

	this->sampleRate = sampleRate;
	this->buffersize = samplesPerBlockExpected;

	if (editor != nullptr) {
		editor->prepareToPlay(samplesPerBlockExpected, sampleRate);
	}
	if (metronome != nullptr) {
		metronome->prepareToPlay(sampleRate, samplesPerBlockExpected);
		metronome->setBPM(Project::getInstance()->getTempo());
	}
}

int MainComponent::getNumActiveChannels(int i) {
	i = i - ((i >> 1) & 0x55555555);
	i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
	return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}


void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
	if (isBouncing()) {
		return;
	}

	// At this point the buffer contains hardware input data (AudioSourcePlayer copies
	// input into the output buffer before calling us). Snapshot it for AudioIn modules,
	// capture levels, then clear so that input does not leak into the speaker output.
	{
		const int nChans = bufferToFill.buffer->getNumChannels();
		const int nSamps = bufferToFill.numSamples;
		inputCaptureBuffer.setSize(nChans, nSamps, false, false, true);
		for (int ch = 0; ch < nChans; ++ch)
			inputCaptureBuffer.copyFrom(ch, 0, *bufferToFill.buffer, ch, bufferToFill.startSample, nSamps);
	}

	if (levelMeter != nullptr)
		levelMeter->processAudioBlock(bufferToFill);

	if (spectrumAnalyzer)
		spectrumAnalyzer->getNextAudioBlock(bufferToFill);

	bufferToFill.clearActiveBufferRegion();

	long startTime = 0;

	if (!isBouncing()) {
		lastTime = (long)juce::Time::getMillisecondCounterHiRes() - currentTime;
		currentTime = (long)juce::Time::getMillisecondCounterHiRes();
		startTime = currentTime;
	}

	const int numSamples = bufferToFill.numSamples;
	auto* writableBuffer = const_cast<juce::AudioBuffer<float>*>(bufferToFill.buffer);
	if (!writableBuffer) return;

	float** outputData = const_cast<float**>(writableBuffer->getArrayOfWritePointers());
	const int outNumChans = writableBuffer->getNumChannels();
	if (!outputData || outNumChans == 0) return;

	// --- Sampler nur auf 0/1 ---
	if (defaultSampler && defaultSampler->isPlaying()) {
		for (int j = 0; j < numSamples; ++j) {
			float sL = defaultSampler->getOutput(0);
			float sR = defaultSampler->getOutput(1);
			if (outNumChans > 0 && outputData[0]) outputData[0][j] += sL;
			if (outNumChans > 1 && outputData[1]) outputData[1][j] += sR;
			defaultSampler->nextSample();
		}
	}

	if (!isRunning()) return;

	if (editor->getModule())
		processModule(editor->getModule());

	auto& mixer = *editor->getMixer();

	std::vector<AudioOut*> outputChannels = mixer.getOutputChannels();
	std::vector<AudioIn*>  inputChannels = mixer.getInputChannels();
	std::vector<AuxOut*>   auxChannels = mixer.getAuxChannels();

	// --- Host -> Mixer Inputs ---
	int numInputPairs = std::min(Mixer::getInstance()->getNumInputs(), (int)inputChannels.size());
	int numAvailableInputChannels = bufferToFill.buffer->getNumChannels();

	for (int k = 0; k < numInputPairs; ++k) {
		Mixer::Channel* inCh = Mixer::getInstance()->getChannel(Mixer::Channel::Type::IN, k);
		if (!inCh || k >= (int)inputChannels.size()) continue;

		// Verwende getSelectedAudioInput() vom jeweiligen AudioIn-Objekt
		int selectedInputIndex = inputChannels.at(k)->getSelectedAudioInput();

		// Prüfen ob der ausgewählte Input-Index verfügbar ist
		if (selectedInputIndex < 0 || selectedInputIndex >= numAvailableInputChannels) continue;

		double pan = inCh->pan;
		float gainL = fastCos((M_PI * (pan + 1) / 4));
		float gainR = fastSin((M_PI * (pan + 1) / 4));

		// AudioBuffer der AudioIn-Objekte holen (das sind die "echten" Buffer)
		auto* bufL = inputChannels.at(k)->pins.at(0)->getAudioBuffer();
		// auto* bufR = inputChannels.at(k)->pins.at(1)->getAudioBuffer();
		if (!bufL) continue;

		// Ausgewählten Input-Kanal vom Host in die AudioIn-Buffer kopieren
		bufL->copyFrom(0, 0, inputCaptureBuffer, selectedInputIndex, 0, numSamples);
		// bufR->copyFrom(0, 0, *bufferToFill.buffer, rightInputCh, bufferToFill.startSample, numSamples);

		// Volume und Pan anwenden
		bufL->applyGain(0, 0, numSamples, inCh->volume * gainL);
		// bufR->applyGain(0, 0, numSamples, inCh->volume * gainR);

		// Pegelberechnung für die Mixer-Anzeige
		inCh->magnitudeLeft = bufL->getMagnitude(0, 0, numSamples);
		//inCh->magnitudeRight = bufR->getMagnitude(0, 0, numSamples);
	}

	// --- Aux Channel Processing ---
	for (int a = 0; a < (int)auxChannels.size(); ++a) {
		Mixer::Channel* auxMeta = Mixer::getInstance()->getChannel(Mixer::Channel::Type::AUX, a);
		if (!auxMeta || !mixer.auxChannelIsValid(a, 0)) continue;

		const float auxVol = (auxMeta && !auxMeta->mute) ? auxMeta->volume : 0.0f;
		const double auxPan = auxMeta ? auxMeta->pan : 0.0;
		const float auxGainL = fastCos((M_PI * (auxPan + 1) / 4));
		const float auxGainR = fastSin((M_PI * (auxPan + 1) / 4));

		auto& auxPins = auxChannels[a]->getPins();

		// Process left and right channels for aux
		for (size_t p = 0; p < auxPins.size() && p < 2; ++p) {
			if (auxPins[p]->getConnections().empty() || !auxPins[p]->getConnections()[0]) continue;
			auto* auxBuf = auxPins[p]->getConnections()[0]->getAudioBuffer();
			if (!auxBuf) continue;

			const float* auxSrc = auxBuf->getReadPointer(0);
			if (!auxSrc) continue;

			// Aux channels are mixed into the main stereo output (hw 0 = L, hw 1 = R).
			int auxHwIndex = (int)p;

			if (auxHwIndex < outNumChans && outputData[auxHwIndex]) {
				float auxGain = (p == 0) ? auxGainL * auxVol : auxGainR * auxVol;
				for (int i = 0; i < numSamples; ++i) {
					outputData[auxHwIndex][i] += auxSrc[i] * auxGain;
				}
			}
		}

		// Calculate magnitude for aux channels
		if (auxMeta && auxPins.size() >= 2) {
			if (auxPins[0]->getConnections().size() > 0)
				auxMeta->magnitudeLeft = auxVol * auxGainL * auxPins[0]->getConnections()[0]->getAudioBuffer()->getMagnitude(0, 0, numSamples);
			if (auxPins[1]->getConnections().size() > 0)
				auxMeta->magnitudeRight = auxVol * auxGainR * auxPins[1]->getConnections()[0]->getAudioBuffer()->getMagnitude(0, 0, numSamples);
		}
	}

	// --- 1:1 Output Mapping mit Pan ---
	for (int o = 0; o < (int)outputChannels.size(); ++o) {
		Mixer::Channel* meta = Mixer::getInstance()->getChannel(Mixer::Channel::Type::OUT, o);
		const float chVol = (meta && !meta->mute) ? meta->volume : 0.0f;
		const double pan = meta ? meta->pan : 0.0;
		const float gainL = fastCos((M_PI * (pan + 1) / 4));
		const float gainR = fastSin((M_PI * (pan + 1) / 4));

		auto& pins = outputChannels[o]->getPins();
		for (size_t p = 0; p < pins.size(); ++p) {
			int hwIndex = o * pins.size() + (int)p; // Annahme: linear hintereinander
			if (hwIndex >= outNumChans) continue;

			if (pins[p]->getConnections().empty() || !pins[p]->getConnections()[0]) continue;
			auto* buf = pins[p]->getConnections()[0]->getAudioBuffer();
			if (!buf) continue;

			const float* src = buf->getReadPointer(0);
			if (!src || !outputData[hwIndex]) continue;

			float gain = (p == 0) ? gainL * chVol : gainR * chVol;
			for (int i = 0; i < numSamples; ++i)
				outputData[hwIndex][i] += src[i] * gain;
		}

		// Pegelberechnung
		if (meta && pins.size() >= 2) {
			if (pins[0]->getConnections().size() > 0)
				meta->magnitudeLeft = chVol * gainL * pins[0]->getConnections()[0]->getAudioBuffer()->getMagnitude(0, 0, numSamples);
			if (pins[1]->getConnections().size() > 0)
				meta->magnitudeRight = chVol * gainR * pins[1]->getConnections()[0]->getAudioBuffer()->getMagnitude(0, 0, numSamples);

		}
	}

	// --- Metronom auf 0/1 ---
	if (metronome && transportDisplay && transportDisplay->isMetronomeActive() && navigator->isRecording()) {
		if (metronomeBuffer.getNumSamples() != bufferToFill.numSamples)
			metronomeBuffer.setSize(2, bufferToFill.numSamples, false, false, true);

		metronome->generateBlock(metronomeBuffer);

		if (outNumChans > 0) {
			auto* destL = bufferToFill.buffer->getWritePointer(0, bufferToFill.startSample);
			auto* srcL = metronomeBuffer.getReadPointer(0);
			for (int i = 0; i < numSamples; ++i) destL[i] += srcL[i];
		}
		if (outNumChans > 1) {
			auto* destR = bufferToFill.buffer->getWritePointer(1, bufferToFill.startSample);
			auto* srcR = metronomeBuffer.getReadPointer(metronomeBuffer.getNumChannels() > 1 ? 1 : 0);
			for (int i = 0; i < numSamples; ++i) destR[i] += srcR[i];
		}
	}

	// Process master FX chain (before master fader)
	if (mixerPanel) {
		auto* masterPan = mixerPanel->getMasterPanel();
		if (masterPan) {
			juce::AudioBuffer<float> masterBuf(outputData, outNumChans, numSamples);
			masterPan->processMasterFX(masterBuf);
		}
	}

	// Apply master channel (volume, mute, pan) to final output
	{
		Mixer::Channel* master = Mixer::getInstance()->getMasterChannel();
		if (master) {
			const float masterVol = master->mute ? 0.0f : (float)master->volume;
			const double masterPan = master->pan;
			const float gainL = fastCos((M_PI * (masterPan + 1) / 4));
			const float gainR = fastSin((M_PI * (masterPan + 1) / 4));

			for (int ch = 0; ch < outNumChans; ++ch) {
				if (!outputData[ch]) continue;
				const float gain = (ch % 2 == 0) ? gainL * masterVol : gainR * masterVol;
				for (int i = 0; i < numSamples; ++i)
					outputData[ch][i] *= gain;
			}

			float magL = 0.0f, magR = 0.0f;
			if (outNumChans > 0 && outputData[0])
				for (int i = 0; i < numSamples; ++i) magL = std::max(magL, std::abs(outputData[0][i]));
			if (outNumChans > 1 && outputData[1])
				for (int i = 0; i < numSamples; ++i) magR = std::max(magR, std::abs(outputData[1][i]));
			master->magnitudeLeft  = magL;
			master->magnitudeRight = magR;
		}
	}

	if (!isBouncing()) {
		long duration = (long)juce::Time::getMillisecondCounterHiRes() - startTime;
		cpuLoad = ((float)duration / (float)lastTime) * 100.0f;
	}

	currentSample = (currentSample + 1) % numSamples;

	if (navigator && (navigator->isPlaying() || navigator->isRecording()))
		navigator->setSamplePosition(navigator->getSamplePosition() + numSamples);
}

void MainComponent::releaseResources()
{
}

//==============================================================================
void MainComponent::paint(Graphics& g)
{
	g.fillAll(Colour(0xff222222));
}

void MainComponent::resized()
{
	if (Project::getInstance()->getAppMode() == Project::AppMode::STUDIO) {
		auto r = getLocalBounds().reduced(4).removeFromBottom(getHeight() - 55);

		if (toolbar != nullptr)
			toolbar->setSize(getLocalBounds().getWidth(), 50);

		// make a list of two of our child components that we want to reposition
		Component* comps[] = { propertyView.get(), &resizerBar, editorView.get() };

		// this will position the 3 components, one above the other, to fit
		// vertically into the rectangle provided.
		stretchableManager.layOutComponents(comps, 3,
			r.getX(), r.getY() + 25, r.getWidth(), r.getHeight(),
			false, true);

		if (propertyView != nullptr && propertyView->getParentComponent() != nullptr)
			propertyView->setSize(r.getWidth() - editorView->getWidth(), propertyView->getHeight());
		if (getParentComponent() != nullptr) {
			if (cpuLoadLabel != nullptr) {
				cpuLoadLabel->setBounds(getParentComponent()->getWidth() - 50, 0, 50, 20);
				loadSlider->setBounds(getParentComponent()->getWidth() - 150, 10, 100, 10);
			}
			if (editorView != nullptr) {
				editorView->getEditor()->resized();
			}
			if (menu != nullptr) {
				menu->setBounds(0, 0, getParentWidth(), 25);
			}

		}
	}
	else {
		if (getParentComponent() != nullptr && editorView != nullptr) {
			editorView->setSize(getParentWidth(), getParentHeight());
			editorView->getEditor()->resized();
		}
	}
}

PopupMenu MainComponent::getMenuForIndex(int index, const String& menuName) {

	PopupMenu menu;

	if (index == 0) {
		menu.addCommandItem(commandManager.get(), SynthEditor::CommandIds::NEW);
		menu.addCommandItem(commandManager.get(), SynthEditor::CommandIds::LOAD);

		// Open Recent submenu — IDs 2000..2099 reserved for recent files so
		// they don't collide with the binary-resource Samples menu (1000+i)
		// further down. The list is loaded fresh every time the menu opens.
		PopupMenu recentMenu;
		Project::getInstance()->loadRecentFileList();
		const StringArray& recent = Project::getInstance()->getRecentFiles();
		for (int i = 0; i < recent.size() && i < 100; ++i)
			recentMenu.addItem(2000 + i, recent.getReference(i));
		menu.addSubMenu("Open Recent", recentMenu, recent.size() > 0);

		menu.addItem(13, "Export Audio...", true, false, nullptr);
		menu.addCommandItem(commandManager.get(), SynthEditor::CommandIds::SAVE);
		menu.addCommandItem(commandManager.get(), SynthEditor::CommandIds::SAVE_AS);
		menu.addItem(5, "Settings", true, false, nullptr);
		menu.addItem(14, "Claude API Key...", true, false, nullptr);
		menu.addItem(6, "Import Audio", true, false, nullptr);
		menu.addItem(12, "Import MIDI", true, false, nullptr);
		menu.addItem(7, "Play", true, false, nullptr);
		PopupMenu samplesMenu = PopupMenu();

		for (int i = 0; i < BinaryData::namedResourceListSize; i++) {
			if (juce::String(BinaryData::namedResourceList[i]).contains("slb")) {
				samplesMenu.addItem(1000 + i, String(BinaryData::namedResourceList[i]));

				int size;
				const char* data = BinaryData::getNamedResource(BinaryData::namedResourceList[i], size);
				String xmlData = String(data);
				sampleData.add(xmlData);
			}
		}

		menu.addSubMenu("Samples", samplesMenu);

		menu.addItem(999, "Exit", true, false, nullptr);
	}
	else if (index == 1) {
#if defined(JUCE_PLUGINHOST_AU) || defined(JUCE_PLUGINHOST_VST)
		menu.addItem(200, "Manage plugins", true, false, nullptr);
#endif
		menu.addSeparator();
		menu.addItem(201, "Tracks", true, false, nullptr);
		menu.addItem(202, "Console", true, false, nullptr);
		menu.addItem(203, "Level Meters", true, false, nullptr);
		menu.addItem(204, "Input Spectrum", true, false, nullptr);
	}
	else if (index == 2) {

#if defined(JUCE_PLUGINHOST_AU) || defined(JUCE_PLUGINHOST_VST)
		if (pluginMenu != nullptr)
			return *pluginMenu;
#endif
	}
	else if (index == 3) {
		menu.addItem(undoCommand, "Undo", true, false, nullptr);
		menu.addItem(redoCommand, "Redo", true, false, nullptr);
		menu.addSeparator();
		menu.addCommandItem(commandManager.get(), SynthEditor::CommandIds::DUPLICATE);
		menu.addCommandItem(commandManager.get(), SynthEditor::CommandIds::DELETE_SELECTED);
		menu.addCommandItem(commandManager.get(), SynthEditor::CommandIds::RESET_GUI_POS);

		PopupMenu alignMenu = PopupMenu();

		alignMenu.addCommandItem(commandManager.get(), SynthEditor::CommandIds::ALIGN_X);
		alignMenu.addCommandItem(commandManager.get(), SynthEditor::CommandIds::ALIGN_Y);
		alignMenu.addCommandItem(commandManager.get(), SynthEditor::CommandIds::ALIGN_TO_GRID);

		menu.addItem(8, "Add Audio Track", true, false, nullptr);
		menu.addItem(9, "Add MIDI Track", true, false, nullptr);
		menu.addItem(10, "Add Instrument Track", true, false, nullptr);
		menu.addItem(11, "Add MIDI Clip", true, false, nullptr);
		menu.addSubMenu("Align", alignMenu);
		menu.addSeparator();
		menu.addItem(80, "Snap to Grid", true,
			Project::getInstance()->isSnapToGridEnabled());
	}
	else if (index == 4) {

		menu.addItem(60, "Delete Track", true, false, nullptr);
		menu.addSeparator();
		menu.addItem(41, "Quantize", true, false, nullptr);
		menu.addItem(43, "Duplicate", true, false, nullptr);
		menu.addItem(44, "Split at marker", true, false, nullptr);
		menu.addItem(49, "Consolidate", true, false, nullptr);
		menu.addItem(45, "Delete", true, false, nullptr);

		PopupMenu audioMenu = PopupMenu();

		audioMenu.addItem(42, "Detect BPM", true, false, nullptr);
		audioMenu.addItem(46, "Reverse", true, false, nullptr);
		audioMenu.addItem(47, "Normalize", true, false, nullptr);
		audioMenu.addItem(48, "Fade In / Out", true, false, nullptr);

		//TODO: implement these functions
		/*

		audioMenu.addItem(50, "Trim", true, false, nullptr);
		audioMenu.addItem(51, "Silence", true, false, nullptr);
		audioMenu.addItem(52, "Time Stretch", true, false, nullptr);
		audioMenu.addItem(53, "Pitch Shift", true, false, nullptr);
		audioMenu.addItem(54, "Apply Gain", true, false, nullptr);
		audioMenu.addItem(55, "Render to new file", true, false, nullptr);
		*/

		menu.addSubMenu("Audio", audioMenu);
	}

	return menu;
}

void MainComponent::menuItemSelected(int menuItemID, int topLevelMenuIndex) {

	if (menuItemID == 5) {
		openSettings();
	}
	else if (menuItemID == 6) {
		editorView->ImportAudio();
	}
	else if (menuItemID == 12) {
		editorView->ImportMidi();
	}
	else if (menuItemID == 13) {
		showBounceDialog();
	}
	else if (menuItemID == 14) {
		showClaudeApiKeyDialog();
	}
	else if (menuItemID == 7) {
		editorView->getNavigator()->setPlaying(true);
	}
	else if (menuItemID == 7) {
		editorView->getNavigator()->setPlaying(false);
	}

#if defined(JUCE_PLUGINHOST_AU) || defined(JUCE_PLUGINHOST_VST)
	else if (menuItemID == 200) {
		PluginManager::getInstance()->scanPlugins();
	}
#endif
	else if (menuItemID == 201) {
		editorView->OpenTrackView();
		if (editorView->getNavigator() != nullptr)
			editorView->getNavigator()->addChangeListener(this);
	}
	else if (menuItemID == 202) {
		editorView->openConsole();
	}

	else if (menuItemID == 203) {
		if (levelMeter == nullptr) {
			levelMeter = std::make_unique<AudioLevelMeter>();
		}
        Project::getInstance()->getSupplemental()->addTab("Level Meter", Colours::darkgrey, levelMeter.get(), false);

	}
	else if (menuItemID == 204) {
		if (spectrumAnalyzer == nullptr) {
			spectrumAnalyzer = std::make_unique<SpectrumAnalyzer>();
			spectrumAnalyzer->setNumberOfBars(128);
		}
        Project::getInstance()->getSupplemental()->addTab("Spektrum", Colours::darkgrey, spectrumAnalyzer.get(), false);
	}

	else if (menuItemID == 8) {

		if (editorView->getNavigator() == nullptr) {
			editorView->OpenTrackView();
			editorView->getNavigator()->addChangeListener(this);
		}
		TrackUtil::createAudioTrack(editor, editorView->getNavigator(), editor->getModule());
		editor->repaint();
	}
	else if (menuItemID == 9) {
		if (editorView->getNavigator() == nullptr) {
			editorView->OpenTrackView();
			editorView->getNavigator()->addChangeListener(this);
		}
		TrackUtil::createMIDITrack(editor, editorView->getNavigator(), editor->getModule());
		editor->repaint();
	}
	else if (menuItemID == 10) {
		if (editorView->getNavigator() == nullptr) {
			editorView->OpenTrackView();
			editorView->getNavigator()->addChangeListener(this);
		}
		editor->addVirtualInstrumentTrack();
		editor->repaint();
	}
	else if (menuItemID == 11)
	{
		if (editorView->getNavigator()->getCurrentTrack() != nullptr)
		{
			const double sr  = Project::getInstance()->getSampleRate();
			const float  bpm = Project::getInstance()->getTempo();
			const int    beatsPerBar = 4;
			const long   oneBarSamples = (long)(beatsPerBar * (60.0 / bpm) * sr);

			editorView->getNavigator()->getCurrentTrack()->addMidiRegion(
				sr,
				editorView->getNavigator()->getSamplePosition(),
				oneBarSamples);
		}
	}
	else if (menuItemID == 80) {
		Project* p = Project::getInstance();
		p->setSnapToGridEnabled(!p->isSnapToGridEnabled());
		// MenuBarModel rebuilds the menu on next open, so the tick state
		// reflects the new value automatically next time it's shown.
	}

	else if (menuItemID == 12) {
		// Implement the handler for Import MIDI
		FileChooser chooser("Select a MIDI file to import...", File(), "*.mid");
		if (chooser.browseForFileToOpen()) {
			File midiFile = chooser.getResult();
			// Add your MIDI import logic here
		}
	}

	else if (menuItemID == 31) {
		editor->duplicateSelected();
	}
#if defined(JUCE_PLUGINHOST_AU) || defined(JUCE_PLUGINHOST_VST)
	else if (menuItemID > 100 && menuItemID < 999) {
		String pluginName = PluginManager::getInstance()->getAvailablePlugins().at(menuItemID - 100);

		if (editor->getSelectionModel().getSelectedModule() != nullptr) {
			PluginModule* pm = dynamic_cast<PluginModule*>(editor->getSelectionModel().getSelectedModule());

			if (pm != nullptr) {
				pm->selectPlugin(pluginName);
			}
		}

	}
#endif

	else if (menuItemID == 41) {

		if (navigator->getCurrentTrack() == nullptr)
			return;

		if (navigator->getCurrentTrack()->getCurrentRegion() == nullptr)
			return;

		{
			// Calculate grid size for 1/16 note at the current tempo and sample rate
			double sr     = Project::getInstance()->getSampleRate();
			float  tempo  = Project::getInstance()->getTempo();
			int gridSamples = (int)(sr * 60.0 / tempo / 4.0);
			navigator->getCurrentTrack()->getCurrentRegion()->quantize(gridSamples);
		}
	}
	else if (menuItemID == 42) {

		if (navigator->getCurrentTrack() == nullptr)
			return;

		if (navigator->getCurrentTrack()->getCurrentRegion() == nullptr)
			return;

		Region* region = navigator->getCurrentTrack()->getCurrentRegion();
		BPMDetector detector;

		if (AudioRegion* ar = dynamic_cast<AudioRegion*>(region)) {
			if (ar->getBuffer() == nullptr) {
				AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon, "Error", "No audio data found in the selected region.");
				return;
			}
			else {
				AudioBuffer<float>* buffer = ar->getBuffer();

				float bpm = detector.detectBPM(*buffer, Project::getInstance()->getSampleRate());
				AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon, "BPM Result", "The region has " + juce::String(bpm) + " bpm");
			}
		}
		else {
			AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon, "Error", "Selected region is not an audio region.");
			return;
		}

	}
	else if (menuItemID == 43) {
		if (navigator->getCurrentTrack() != nullptr) {
			Region* region = navigator->getCurrentTrack()->getCurrentRegion();
			if (region != nullptr) {
				navigator->getCurrentTrack()->duplicateRegion(region);
			}
		}
	}
	else if (menuItemID == 44) {
		if (navigator->getCurrentTrack() != nullptr) {

			// split at current position, for every track
			// iterate through all tracks

			for (int i = 0; i < navigator->getTracks().size(); i++) {
				navigator->getTracks().at(i)->splitRegion();
			}
				
		}
	}
	else if (menuItemID == 45) {
		for (int i = 0; i < navigator->getTracks().size(); i++) {
			navigator->getTracks().at(i)->removeSelectedRegions(true);
		}
	}
	else if (menuItemID == 46) {
		if (navigator->getCurrentTrack() != nullptr) {
			Region* region = navigator->getCurrentTrack()->getCurrentRegion();
			// check if it's an audio region
			AudioRegion* audioRegion = dynamic_cast<AudioRegion*>(region);

			if (audioRegion != nullptr) {
				AudioUtils::reverseAudioBuffer(*audioRegion->getBuffer());
				audioRegion->updateThumb();
			}

		}
	}
	else if (menuItemID == 47) {
		NormalizeRegionAction* action = new NormalizeRegionAction(navigator);
		Project::getInstance()->getUndoManager()->beginNewTransaction();
		Project::getInstance()->getUndoManager()->perform(action);

	}
	else if (menuItemID == 48) {
		FadeRegionAction* action = new FadeRegionAction(navigator);
		Project::getInstance()->getUndoManager()->beginNewTransaction();
		Project::getInstance()->getUndoManager()->perform(action);
	}
	else if (menuItemID == 49) {
		if (navigator->getCurrentTrack() != nullptr) {
			Region* region = navigator->getCurrentTrack()->getCurrentRegion();
			if (region != nullptr)
				region->consolidate();
		}
	}
	else if (menuItemID == 60) {
		if (navigator != nullptr && navigator->getCurrentTrack() != nullptr) {
			auto* action = new TrackDeleteAction(navigator, editor);
			Project::getInstance()->getUndoManager()->beginNewTransaction();
			Project::getInstance()->getUndoManager()->perform(action);
		}
	}
	else if (menuItemID == undoCommand) {
		Project::getInstance()->getUndoManager()->undo();
	}
	else if (menuItemID == redoCommand) {
		Project::getInstance()->getUndoManager()->redo();
	}
	else if (menuItemID == 999) {
		JUCEApplication::getInstance()->shutdown();
	}
	else if (menuItemID >= 2000 && menuItemID < 2100) {
		// Open Recent — must run BEFORE the >=1000 samples branch below,
		// otherwise the catch-all there would eat these IDs.
		const StringArray& recent = Project::getInstance()->getRecentFiles();
		const int idx = menuItemID - 2000;
		if (idx >= 0 && idx < recent.size()) {
			juce::File file(recent.getReference(idx));
			editor->setRunning(false);
			editor->openFile(file);
			editor->setRunning(true);

			// Same Navigator-tab restoration as the toolbar Open path —
			// SynthEditor::cleanUp() strips every tab except Editor; if the
			// loaded project has tracks, recreate the Navigator tab so the
			// user lands on the arrange view.
			juce::Component::SafePointer<MainComponent> safeThis(this);
			juce::Timer::callAfterDelay(150, [safeThis]() {
				if (auto* self = safeThis.getComponent()) {
					if (self->editorView != nullptr
						&& self->editorView->getNavigator() != nullptr
						&& !self->editorView->getNavigator()->getTracks().empty())
					{
						self->editorView->OpenTrackView();
					}
				}
			});
		}
	}
	else if (menuItemID >= 1000) {
		String xmlData = sampleData.getReference(menuItemID - 1000);
		editor->setRunning(false);
		editor->cleanUp();
		editor->newFile();
		editor->loadFromString(xmlData);
		editor->setRunning(true);
	}
	
}

StringArray MainComponent::getMenuBarNames() {

#if defined(JUCE_PLUGINHOST_AU) || defined(JUCE_PLUGINHOST_VST)
	const char* const names[] = { "File", "View","Plugins", "Edit", "Region", nullptr };
#else
	const char* const names[] = { "File", "View", "Edit", nullptr };
#endif

	return StringArray(names);

}

void MainComponent::openSettings() {

	AudioDeviceSelectorComponent* selector = new AudioDeviceSelectorComponent(deviceManager, 2, 16, 2, 16, true, true, true, false);
	DialogWindow::LaunchOptions launchOptions;

	selector->setLookAndFeel(Project::getInstance()->getLookAndFeel());
	launchOptions.dialogTitle = ("Audio Settings");
	launchOptions.escapeKeyTriggersCloseButton = true;
	launchOptions.resizable = false;
	launchOptions.useNativeTitleBar = false;
	launchOptions.useBottomRightCornerResizer = true;
	launchOptions.componentToCentreAround = getParentComponent();
	launchOptions.content.setOwned(selector);
	launchOptions.content->setSize(600, 580);
	launchOptions.dialogBackgroundColour = Colour(0xff222222);

	DialogWindow* window = launchOptions.launchAsync();

	std::function<void(int)> lambda =
		[this, selector](int result) {
		AudioDeviceManager::AudioDeviceSetup setup;

		deviceManager.getAudioDeviceSetup(setup);
		deviceManager.restartLastAudioDevice();
		AudioManager::getInstance()->setupChannels();
		midiProcessor->refreshMidiInputs();

		std::unique_ptr<XmlElement> config = deviceManager.createStateXml();

		String userHome = File::getSpecialLocation(File::userHomeDirectory).getFullPathName();

		File appDir = File(userHome + "/.Synthlab");

		if (!appDir.exists()) {
			appDir.createDirectory();
		}

		File configFile = File(userHome + "/.Synthlab/config.xml");

		if (config != nullptr) {
			config->writeToFile(configFile, "");
			config = nullptr;
		}
		Project::getInstance()->setSampleRate(setup.sampleRate);
		delete selector;

		};

	ModalComponentManager::Callback* callback = ModalCallbackFunction::create(lambda);
	ModalComponentManager::getInstance()->attachCallback(window, callback);

	Project::getInstance()->getOpenWindows().push_back(window);

}

static juce::File getClaudeApiKeyFile()
{
	const juce::String userHome = juce::File::getSpecialLocation(juce::File::userHomeDirectory).getFullPathName();
	juce::File appDir(userHome + "/.Synthlab");
	if (!appDir.exists())
		appDir.createDirectory();
	return juce::File(userHome + "/.Synthlab/claude_api_key.txt");
}

juce::String MainComponent::loadClaudeApiKey() const
{
	const juce::File f = getClaudeApiKeyFile();
	if (!f.existsAsFile())
		return {};
	return f.loadFileAsString().trim();
}

void MainComponent::saveClaudeApiKey(const juce::String& key)
{
	const juce::File f = getClaudeApiKeyFile();
	f.replaceWithText(key.trim());
	if (chat != nullptr)
		chat->setApiKey(key.trim());
}

void MainComponent::showClaudeApiKeyDialog()
{
	auto* w = new juce::AlertWindow(
		"Claude API Key",
		"Trage hier deinen Anthropic API Key ein. Er wird in ~/.Synthlab/claude_api_key.txt gespeichert.",
		juce::AlertWindow::NoIcon);

	w->setLookAndFeel(Project::getInstance()->getLookAndFeel());
	w->addTextEditor("apiKey", loadClaudeApiKey(), "API Key:", true /* isPassword */);
	w->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey));
	w->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

	w->enterModalState(true,
		juce::ModalCallbackFunction::create([this, w](int result)
		{
			if (result == 1)
			{
				const juce::String newKey = w->getTextEditorContents("apiKey");
				saveClaudeApiKey(newKey);
			}
		}),
		true /* deleteWhenDismissed */);
}

bool MainComponent::keyStateChanged(bool isKeyDown, juce::Component* originatingComponent) {

	// TODO : If we provide Keyboard INput for play, we shoukd clean this up!

	/*
	for (std::map<int, int>::iterator it = keyCodeMidiNote.begin(); it != keyCodeMidiNote.end(); it++) {
		if (KeyPress::isKeyCurrentlyDown((*it).first)) {
			if (!keyStates[(*it).second]) {
				sendNoteMessage(editor->getModule(), 1, (*it).second + 12 * currentOctave);
				sendGateMessage(editor->getModule(), 1, 64, (*it).second + 12 * currentOctave, true);
				keyStates[(*it).second] = true;
			}
		}
		else {
			keyStates[(*it).second] = false;
			if (!isKeyDown) {
				sendGateMessage(editor->getModule(), 1, 0, (*it).second + 12 * currentOctave, false);
			}
		}
	}
	*/
	return true;
}

void MainComponent::modifierKeysChanged(const juce::ModifierKeys& modifiers) {
	isOptionPressed = modifiers.isAltDown();
}

bool MainComponent::keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) {
	if (key.getTextCharacter() == 'y') {
		if (currentOctave > 0) {
			currentOctave--;
		}
	}
	if (key.getTextCharacter() == 'x') {
		if (currentOctave < 8) {
			currentOctave++;
		}
	}
	if (key.getTextCharacter() == 'l') {
		layer = (layer + 1) % 4;
		editor->setCurrentLayer(layer);
		editor->repaint();
	}
	if (key.getTextCharacter() == 'd' && isOptionPressed) {
		editor->duplicateSelected();
		editor->repaint();
	}
	if (key.getKeyCode() == KeyPress::deleteKey || key.getKeyCode() == KeyPress::backspaceKey) {
		RemoveSelectedAction* rsa = new RemoveSelectedAction(editor);
		Project::getInstance()->getUndoManager()->beginNewTransaction();
		Project::getInstance()->getUndoManager()->perform(rsa);
	}

	return true;
}


void MainComponent::createKeyMap() {
	keyCodeMidiNote['a'] = 36;
	keyCodeMidiNote['w'] = 36;
	keyCodeMidiNote['s'] = 38;
	keyCodeMidiNote['e'] = 39;
	keyCodeMidiNote['d'] = 40;
	keyCodeMidiNote['f'] = 41;
	keyCodeMidiNote['t'] = 42;
	keyCodeMidiNote['g'] = 43;
	keyCodeMidiNote['z'] = 44;
	keyCodeMidiNote['h'] = 45;
	keyCodeMidiNote['u'] = 46;
	keyCodeMidiNote['j'] = 47;
	keyCodeMidiNote['k'] = 48;
	keyCodeMidiNote['o'] = 49;

}

void MainComponent::buttonClicked(Button* b)
{
	ToolbarButton* tb = dynamic_cast<ToolbarButton*>(b);

	if (tb != nullptr) {

		if (tb->getItemId() == toolbarFactory->delete_element) {
			RemoveSelectedAction* rma = new RemoveSelectedAction(editor);
			Project::getInstance()->getUndoManager()->beginNewTransaction();
			Project::getInstance()->getUndoManager()->perform(rma);
		}
		else if (tb->getItemId() == toolbarFactory->doc_new) {
			editor->setRunning(false);
			Project::getInstance()->setNewFile(true);
			navigator->clearTracks();
			editor->cleanUp();
			editor->newFile();
			editor->setRunning(true);
		}
		else if (tb->getItemId() == toolbarFactory->doc_save) {
			editor->saveFile();
		}
		else if (tb->getItemId() == toolbarFactory->doc_open) {
			editor->setRunning(false);
			editor->openFile();
			editor->setRunning(true);

			// SynthEditor::cleanUp() (called at the start of openFile) strips every
			// tab except the Editor tab. If the freshly loaded project actually has
			// tracks, recreate the Navigator tab and bring it to the front so the
			// user lands on the arrange view. Deferred via callAfterDelay so any
			// pending layout/region thumbnail work from openTracks() can settle
			// first; without the delay the navigator can come up empty or
			// mis-sized on heavier projects.
			juce::Component::SafePointer<MainComponent> safeThis(this);
			juce::Timer::callAfterDelay(150, [safeThis]() {
				if (auto* self = safeThis.getComponent()) {
					if (self->editorView != nullptr
						&& self->editorView->getNavigator() != nullptr
						&& !self->editorView->getNavigator()->getTracks().empty())
					{
						self->editorView->OpenTrackView();
					}
				}
			});
		}
		else if (tb->getItemId() == toolbarFactory->app_settings) {
			openSettings();
		}
		else if (tb->getItemId() == toolbarFactory->app_undo) {
			Project::getInstance()->getUndoManager()->undo();
		}
		else if (tb->getItemId() == toolbarFactory->app_redo) {
			Project::getInstance()->getUndoManager()->redo();
		}
		else if (tb->getItemId() == toolbarFactory->mod_sources) {

			if (moduleBrowser == nullptr) {
				moduleBrowser = std::make_unique<ModuleBrowser>();
				addChildComponent(moduleBrowser.get());
			}

			if (!moduleBrowser->isVisible()) {
				moduleBrowser->setTopLeftPosition(b->getX(), b->getY() + b->getHeight());
				moduleBrowser->setVisible(true);
				moduleBrowser->toFront(true);
			}
			else {
				moduleBrowser->setVisible(false);
			}

		}
		else if (tb->getItemId() == toolbarFactory->app_stop) {
			if (navigator != nullptr) {
				if (navigator->isPlaying() || navigator->isRecording()) {
					navigator->setPlaying(false);
					navigator->setRecording(false);
					metronome->reset();
					for (int i = 0; i < editor->getModule()->getModules()->size(); i++) {

						for (int j = 0; j < 16; j++) {
							midiProcessor->sendMidiStopMessage(editor->getModule()->getModules()->at(i), j);

							for (int k = 0; k < 128; k++) {
								midiProcessor->sendGateMessage(editor->getModule()->getModules()->at(i), j, 0, k, false);
							}
						}

						PluginModule* module = nullptr;

						if ((module = dynamic_cast<PluginModule*>(editor->getModule()->getModules()->at(i))) != nullptr) {
							module->getPlayHead()->setPlaying(false);
							module->getPlayHead()->setRecording(false);
							module->resetPlugin();
						}
					}
				}
				else {
					navigator->ResetMarkerPosition();
				}
			}

		}
		else if (tb->getItemId() == toolbarFactory->app_play) {
			if (navigator != nullptr)
				navigator->setPlaying(true);

			for (int i = 0; i < editor->getModule()->getModules()->size(); i++) {

				PluginModule* module = nullptr;

				if ((module = dynamic_cast<PluginModule*>(editor->getModule()->getModules()->at(i))) != nullptr) {
					module->getPlayHead()->setPlaying(true);
				}
				else {
					midiProcessor->sendMidiStartMessage(editor->getModule()->getModules()->at(i), 1);
				}
			}

		}
		else if (tb->getItemId() == toolbarFactory->app_record) {
			if (navigator != nullptr)
				navigator->setRecording(true);
            
            for (int i = 0; i < editor->getModule()->getModules()->size(); i++) {

                PluginModule* module = nullptr;

                if ((module = dynamic_cast<PluginModule*>(editor->getModule()->getModules()->at(i))) != nullptr) {
                    module->getPlayHead()->setPlaying(false);
                    module->getPlayHead()->setRecording(true);
                    module->resetPlugin();
                }
            }
            
		}
		else if (tb->getItemId() == toolbarFactory->app_fast_backwards) {
			if (navigator != nullptr)
				navigator->ResetMarkerPosition();
		}

	}

	if (b == lockButton) {
		editor->setLocked(lockButton->getToggleState());
	}

}

void MainComponent::processModule(Module* m) {

	if (m != nullptr && isRunning()) {

		m->process();

		for (int i = 0; i < m->getModules()->size(); i++) {
			if (m->getModules()->at(i) != nullptr) {
				processModule(m->getModules()->at(i));
			}
		}
	}
}

void MainComponent::changeListenerCallback(juce::ChangeBroadcaster* source) {

	TrackNavigator* navigator = dynamic_cast<TrackNavigator*>(source);
	if (navigator != nullptr) {
		currentSample = navigator->getSamplePosition();
	}
	if (midiProcessor != nullptr) {
		if (navigator != nullptr && navigator->getCurrentTrack() != nullptr)
			midiProcessor->setActiveChannel(navigator->getCurrentTrack()->getMidiChannel());
	}

}

void MainComponent::comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) {
	int id = comboBoxThatHasChanged->getSelectedId() - 1;
	editor->setCurrentLayer(id);
}

void MainComponent::selectionChanged(juce::Component* m) {

}

void MainComponent::fileChanged(juce::String name) {
	lockButton->setToggleState(editor->isLocked(), juce::NotificationType::dontSendNotification);
	if (layerCombobox != nullptr)
		layerCombobox->getComboBox().setSelectedId(editor->getCurrentLayer() + 1);
}

void MainComponent::dirtyChanged(bool dirty) {

}

void MainComponent::startBounce(const juce::File& outputFile, double durationInSeconds)
{
	bounceProgress = 0.0;

	bounceThread = std::make_unique<BounceThread>(
		this, outputFile, durationInSeconds,
		[this](float p) { bounceProgress.store(p); }
	);
	bounceThread->startThread();

	auto* content = new BounceProgressDialog(
		bounceProgress,
		[this]() { return !bounceThread || !bounceThread->isThreadRunning(); }
	);

	// Use launchAsync so the DialogWindow manages its own lifetime.
	// BounceProgressDialog's timer calls exitModalState(0) when done,
	// which causes the async dialog to close and delete itself safely.
	juce::DialogWindow::LaunchOptions options;
	options.dialogTitle = "Exportiere Audio...";
	options.dialogBackgroundColour = juce::Colours::black;
	options.useNativeTitleBar = true;
	options.resizable = false;
	options.content.setOwned(content);
	options.content->setSize(400, 80);
	options.launchAsync();
}

juce::String MainComponent::tryExecuteChatInstruction(const std::string& jsonString)
{
	// Claude (und auch GPT) verpacken JSON gerne in ```json ... ``` oder
	// umgeben es mit Erklärtext, selbst wenn der Prompt strikt JSON verlangt.
	// Wir extrahieren deshalb das erste vollständige Objekt {…} per Klammer-
	// Zähler, statt den rohen String an JSON::parse zu geben.
	auto extractFirstJsonObject = [](const juce::String& s) -> juce::String
	{
		const int first = s.indexOfChar('{');
		if (first < 0)
			return {};

		int depth = 0;
		bool inString = false;
		bool escape = false;
		for (int i = first; i < s.length(); ++i)
		{
			const juce_wchar c = s[i];
			if (escape) { escape = false; continue; }
			if (inString) {
				if (c == '\\') escape = true;
				else if (c == '"') inString = false;
				continue;
			}
			if (c == '"') { inString = true; continue; }
			if (c == '{') ++depth;
			else if (c == '}') {
				--depth;
				if (depth == 0)
					return s.substring(first, i + 1);
			}
		}
		return {};
	};

	const juce::String cleaned = extractFirstJsonObject(juce::String(jsonString));
	if (cleaned.isEmpty())
		return "Claude-Antwort enthält kein JSON-Objekt.";

	juce::var parsed = juce::JSON::parse(cleaned);
	if (!parsed.isObject())
		return "Claude-Antwort konnte nicht als JSON-Objekt geparst werden.";

	// Action-String normalisieren — Claude ist beim genauen Format unzuverlässig
	// (mal "create_midi_region", mal "createMidiRegion", mal "create-midi-region").
	// Vergleich daher case-insensitiv und ohne Trennzeichen.
	const juce::String rawAction = parsed["action"].toString();
	juce::String normAction = rawAction.toLowerCase();
	normAction = normAction.replace("_", "").replace("-", "").replace(" ", "");

	const bool isCreateMidiRegion =
		normAction == "createmidiregion" || normAction.contains("midiregion");

	// Fallback: wenn ein notes-Array vorhanden ist, behandeln wir die Antwort
	// als create_midi_region — egal wie die action heißt. Aktuell ist das die
	// einzige unterstützte Aktion, also lieber nachsichtig sein.
	const bool hasNotesArray = parsed.getProperty("notes", {}).isArray();

	if (isCreateMidiRegion || hasNotesArray)
	{
		double length = parsed["length_beats"];
		juce::Array<juce::var>* notes = parsed.getProperty("notes", {}).getArray();
		if (notes == nullptr) return "Keine Noten gefunden.";

		std::vector<juce::var> sortedNotes;
		for (const auto& n : *notes)
			sortedNotes.push_back(n);

		std::sort(sortedNotes.begin(), sortedNotes.end(),
			[](const juce::var& a, const juce::var& b)
			{
				return static_cast<double>(a["start"]) < static_cast<double>(b["start"]);
			});


		auto* track = navigator->getCurrentTrack();
		if (!track) return "Kein aktiver Track.";

		auto sampleRate = 44100.0;
		auto* region = track->addMidiRegion(sampleRate, navigator->getSamplePosition(), length * sampleRate);

		for (auto& n : *notes)
		{
			int pitch = n["pitch"];
			float start = n["start"];
			float len = n["length"];

			MidiMessage noteOn = MidiMessage(MidiMessage::noteOn(track->getMidiChannel(), pitch, (uint8)100));
			noteOn.setTimeStamp(start * 44100);

			MidiMessage noteOff = MidiMessage::noteOff(track->getMidiChannel(), pitch, (uint8)0);
			noteOff.setTimeStamp((start + len) * 44100);

			region->addMessage(new MidiMessage(noteOn), Time::getMillisecondCounterHiRes() / 1000, (int)(start * 44100));
			region->addMessage(new MidiMessage(noteOff), Time::getMillisecondCounterHiRes() / 1000, (int)((start + len) * 44100));
		}

		return "MIDI-Region erstellt: " + juce::String(notes->size())
			+ " Noten, " + juce::String(length, 1) + " beats.";
	}

	return "Unbekannte Aktion: \"" + rawAction + "\".";
}

juce::ApplicationCommandTarget* MainComponent::getNextCommandTarget()
{
	return editor;
}

void MainComponent::getAllCommands(Array<CommandID>& commands)
{
	commands.add(undoCommand);
	commands.add(redoCommand);
}

void MainComponent::getCommandInfo(CommandID commandID, ApplicationCommandInfo& result)
{
	switch (commandID)
	{
	case undoCommand:
		result.setInfo("Undo", "Undo: " + Project::getInstance()->getCommandManager()->getDescriptionOfCommand(undoCommand), "General", 0);
		result.addDefaultKeypress('z', juce::ModifierKeys::ctrlModifier);
		break;

	case redoCommand:
		result.setInfo("Redo", "Redo: " + Project::getInstance()->getCommandManager()->getDescriptionOfCommand(redoCommand), "General", 0);
		result.addDefaultKeypress('y', juce::ModifierKeys::ctrlModifier);
		break;
	}
}

bool MainComponent::perform(const InvocationInfo& info)
{
	switch (info.commandID)
	{
	case undoCommand:
		Project::getInstance()->getUndoManager()->undo();
		commandManager->commandStatusChanged();
		return true;

	case redoCommand:
		Project::getInstance()->getUndoManager()->redo();
		commandManager->commandStatusChanged();
		return true;
	}
	return false;
}


void MainComponent::showBounceDialog()
{
	auto* dialog = new BounceDialog();

	dialog->onExport = [this, dialog]() {
		// Validierung
		if (!dialog->getOutputFile().existsAsFile() && dialog->getOutputFile().getFullPathName().isEmpty()) {
			juce::AlertWindow::showMessageBoxAsync(
				juce::AlertWindow::WarningIcon,
				"Export Error",
				"Please select an output file first.");
			return;
		}

		double duration = dialog->getDuration();
		if (duration <= 0.0) {
			juce::AlertWindow::showMessageBoxAsync(
				juce::AlertWindow::WarningIcon,
				"Export Error",
				"Duration must be greater than 0.");
			return;
		}

		// Bereite Bounce vor
		setRunning(false);
		setBouncing(true);

		// Alle Plugin-Module für Bounce-Modus vorbereiten
		for (int i = 0; i < editor->getModule()->getModules()->size(); i++) {
			if (dynamic_cast<TrackIn*>(editor->getModule()->getModules()->at(i)) != nullptr) {
				TrackIn* t = dynamic_cast<TrackIn*>(editor->getModule()->getModules()->at(i));
				t->setBouncing(true);
			}
		}

		// Starte Bounce
		startBounce(dialog->getOutputFile(), duration);

		// Schließe Dialog
		if (auto* dw = dialog->findParentComponentOfClass<juce::DialogWindow>()) {
			dw->exitModalState(0);
		}
		};

	dialog->onCancel = [dialog]() {
		if (auto* dw = dialog->findParentComponentOfClass<juce::DialogWindow>()) {
			dw->exitModalState(0);
		}
		};

	juce::DialogWindow::LaunchOptions options;
	options.content.setOwned(dialog);
	options.dialogTitle = "Export Audio";
	options.dialogBackgroundColour = juce::Colour(0xff222222);
	options.escapeKeyTriggersCloseButton = true;
	options.useNativeTitleBar = true;
	options.resizable = false;
	options.componentToCentreAround = this;

	auto* window = options.launchAsync();
	Project::getInstance()->getOpenWindows().push_back(window);
}
