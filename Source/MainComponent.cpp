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
#include "TerminalModule.h"
#include "PrefabFactory.h"
#include "Project/Project.h"
#include "Knob.h"
#include "Mixer.h"
#include "BusOut.h"
#include "AudioOut.h"
#include "AuxOut.h"
#include "Tracks/MasterChannelPanel.h"
#include "Actions/RemoveSelectedAction.h"
#include "Actions/AddModuleAction.h"
#include "ModuleBrowser.h"
#include "PitchBendModule.h"
#include "ModuleUtils.h"
#include "MidiClock.h"
#include "MidiRecorderModule.h"
#include "TrackUtil.h"
#include "Tracks/BounceThread.h"
#include "Tracks/Track.h"
#include "Tracks/TrackNavigator.h"
#include "Session.h"
#include "AudioSettingsDialog.h"
#include "BounceDialog.h"
#include "FadeDialog.h"
#include "GainDialog.h"
#include "SilenceDialog.h"
#include "TrimDialog.h"
#include "ChatGPTInterface.h"
#include "StatusBarComponent.h"
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

	// Show the Quickstart wizard tab on first start (or whenever the user
	// has the "Show on startup" toggle enabled). Deferred so the editor tab
	// layout is fully realised before we add to it.
	if (Project::getInstance()->getAppMode() == Project::AppMode::STUDIO
		&& QuickstartComponent::getShowOnStartup())
	{
		juce::MessageManager::callAsync([this]() {
			openQuickstart();
		});
	}
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
		// Reihenfolge: editorView ZUERST, dann propertyView. editorView=nullptr
		// triggert ~EditorComponent -> ~SynthEditor -> cleanUp(), das u.a.
		// Project::getInstance()->getPropertyView()->selectionChanged() ruft.
		// Wenn propertyView vorher schon nullptr (also tot) ist, hält Project
		// nur noch einen Dangling-Raw-Pointer (gesetzt in createStudioLayout)
		// und der Aufruf crasht mit UAF.
		editorView = nullptr;
		// Project's Pointer auf propertyView nullen, BEVOR der unique_ptr
		// freigegeben wird — sonst hält Project einen Dangling-Pointer.
		if (auto* proj = Project::getInstance())
			proj->setPropertyView(nullptr);
		propertyView = nullptr;

		tab = nullptr;
		delete lockButton;
		menu = nullptr;
		toolbar = nullptr;
		toolbarFactory = nullptr;
#if defined(JUCE_PLUGINHOST_AU)
		delete pluginMenu;
#endif
		delete cpuLoadLabel;
		delete loadSlider;

	}
	else if (Project::getInstance()->getAppMode() == Project::AppMode::PLAYER) {
		delete editor;
		editorView = nullptr;
		if (auto* proj = Project::getInstance())
			proj->setPropertyView(nullptr);
		propertyView = nullptr;
		tab = nullptr;
		toolbarFactory = nullptr;
		mixerPanel = nullptr;
		delete lockButton;
	}

	if (defaultSampler != nullptr) {
		delete defaultSampler;
	}

	stopTimer();

	delete midiProcessor;
	chat = nullptr;

	// PrefabFactory::destroy()/Project::destroy() bewusst NICHT hier — die
	// Member-Destruktion (insb. shared_ptr<SynthEditor>, der oft noch ueber
	// Tabs/PluginManager am Leben bleibt) laeuft NACH dem dtor-Body und ruft
	// SynthEditor::cleanUp(), das Project::getInstance() braucht. Lief das
	// destroy hier, sah getInstance() nullptr und legte eine frische, halb-
	// initialisierte Project-Instanz an — naechster Method-Aufruf darauf =
	// EXC_BAD_ACCESS. Singleton-Tear-Down passiert jetzt in
	// SynthlabApplication::shutdown nach `delete mainWindow`.
	if (auto* p = Project::getInstance())
		p->removeAllChangeListeners();

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
	statusBar = std::make_unique<StatusBarComponent>();
	addAndMakeVisible(statusBar.get());

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

		// Status-Leiste aktualisieren. CPU/Tempo/Audio sind billig — jeden
		// Tick. Process-Memory zieht einen Syscall, daher nur alle ~10 Ticks
		// (=~ 2.5 Sekunden bei 40 ms Timer).
		if (statusBar != nullptr) {
			statusBar->setCpuLoad(cpuLoad);
			statusBar->setAudioInfo(sampleRate, buffersize);
			statusBar->setTempo(Project::getInstance()->getTempo());

			static int memTickCounter = 0;
			if ((memTickCounter++ % 10) == 0)
				statusBar->setMemoryUsageMB(StatusBarComponent::getProcessMemoryUsageMB());
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

	if (auto* rootModule = editor->getModule()) {
		// Re-seed the root every block so it always reflects the live device
		// settings. Without this, loading a patch (which replaces `root` via
		// SynthEditor::setModule) leaves the new root at the header default
		// (44100/512) until the user manually toggles the audio device — the
		// classic "sounds wrong until I change the sample rate" symptom.
		// processModule then propagates the correct values down to every
		// child including freshly-created TerminalModules.
		rootModule->setSampleRate((float)sampleRate);
		rootModule->setBuffersize(numSamples);
		processModule(rootModule);
	}

	auto& mixer = *editor->getMixer();

	const std::vector<AudioOut*>& outputChannels = mixer.getOutputChannels();
	const std::vector<AudioIn*>&  inputChannels  = mixer.getInputChannels();
	const std::vector<AuxOut*>&   auxChannels    = mixer.getAuxChannels();

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
				juce::FloatVectorOperations::addWithMultiply(outputData[auxHwIndex], auxSrc, auxGain, numSamples);
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

	// --- Bus Channel Processing ---
	// Anders als AuxOut routet BusOut nicht ueber Patch-Kabel. Statt dessen
	// halten Tracks eine Send-Liste; pro Block sammeln wir hier alle Track-
	// Sends in die BusOut-internen Mix-Buffer und mischen den Bus dann mit
	// eigenem Volume/Pan/Mute auf den Master. Layout:
	//   1) jeden Bus-Buffer leeren (Steady-State: kein Realloc)
	//   2) pro Track-Strip: AudioOut->pin[0/1] Buffer = post-FX, post-fader
	//      Signal; pro Send das in den Ziel-Bus mischen
	//   3) Bus-Buffer auf Master summieren wie ein AuxOut
	const std::vector<BusOut*>& busChannels = mixer.getBusChannels();
	if (! busChannels.empty()) {
		// Schritt 1: alle Bus-Mix-Buffers leeren.
		for (auto* b : busChannels) {
			if (b != nullptr) b->clearMixBuffer(numSamples);
		}

		// Schritt 2: Tracks pro Send in Bus-Buffer mischen.
		// Bus-Lookup: busModuleIndex -> BusOut*. Lineare Suche reicht
		// (typisch <8 Busse), und der Audio-Thread iteriert nur durchs
		// flache busChannels-Array.
		auto findBusByModuleIndex = [&](long idx) -> BusOut* {
			for (auto* b : busChannels) {
				if (b != nullptr && (long) b->getIndex() == idx) return b;
			}
			return nullptr;
		};

		for (auto* panel : mixer.getChannelStrips()) {
			if (panel == nullptr) continue;
			Track* t = panel->getTrack();
			if (t == nullptr) continue;
			const auto& sends = t->getSends();
			if (sends.empty()) continue;

			// Output-Modul des Strips besorgen — Tracks koennen je nach
			// Settings::getNewTrackOutputType entweder mit AudioOut ODER
			// AuxOut als Strip-Output angelegt sein (TrackUtil::createAudioTrack,
			// preferAuxOut-Pfad). Beide haben identische L/R IN-Pin-Layouts;
			// vor dem Fix akzeptierte die Schleife nur AudioOut und sprang
			// fuer AuxOut-Tracks mit continue — Folge: Bus-VU blieb bei
			// Tracks im AuxOut-Modus stumm, obwohl der Send eingetragen war.
			Module* outMod = nullptr;
			if (auto* ao  = dynamic_cast<AudioOut*>(panel->getModule()))
				outMod = ao;
			else if (auto* axo = dynamic_cast<AuxOut*>(panel->getModule()))
				outMod = axo;
			if (outMod == nullptr) continue;
			auto& outPins = outMod->getPins();
			if (outPins.size() < 2) continue;

			const float* srcL = nullptr;
			const float* srcR = nullptr;
			if (! outPins[0]->getConnections().empty()
				&& outPins[0]->getConnections()[0] != nullptr
				&& outPins[0]->getConnections()[0]->getAudioBuffer() != nullptr)
				srcL = outPins[0]->getConnections()[0]->getAudioBuffer()->getReadPointer(0);
			if (! outPins[1]->getConnections().empty()
				&& outPins[1]->getConnections()[0] != nullptr
				&& outPins[1]->getConnections()[0]->getAudioBuffer() != nullptr)
				srcR = outPins[1]->getConnections()[0]->getAudioBuffer()->getReadPointer(0);

			if (srcL == nullptr && srcR == nullptr) continue;

			for (const auto& s : sends) {
				BusOut* bus = findBusByModuleIndex(s.busModuleIndex);
				if (bus == nullptr) continue;
				auto* mb = bus->getMixBuffer();
				if (mb == nullptr || mb->getNumChannels() < 2 || mb->getNumSamples() < numSamples) continue;
				const float amt = s.amount;
				if (amt <= 0.0f) continue;

				if (srcL != nullptr)
					juce::FloatVectorOperations::addWithMultiply(mb->getWritePointer(0), srcL, amt, numSamples);
				if (srcR != nullptr)
					juce::FloatVectorOperations::addWithMultiply(mb->getWritePointer(1), srcR, amt, numSamples);
			}
		}

		// Schritt 3: Bus-FX-Chain auf mixBuffer anwenden (vor Volume/Pan),
		// danach Bus-Buffer mit eigenem Volume/Pan/Mute auf Master.
		const juce::int64 transportPos  = navigator != nullptr ? navigator->getSamplePosition() : 0;
		const bool        isPlayingNow  = navigator != nullptr && navigator->isPlaying();
		const bool        isRecordingNow= navigator != nullptr && navigator->isRecording();

		for (int b = 0; b < (int) busChannels.size(); ++b) {
			BusOut* bus = busChannels[b];
			if (bus == nullptr) continue;
			Mixer::Channel* busMeta = Mixer::getInstance()->getChannel(Mixer::Channel::Type::BUS, b);
			if (busMeta == nullptr) continue;

			auto* mb = bus->getMixBuffer();
			if (mb == nullptr || mb->getNumSamples() < numSamples) continue;

			// FX-Chain dieses Busses laufen lassen (Plugins liegen ausserhalb
			// des Modul-Graphen, gleiches Pattern wie Master-FX). Lookup ueber
			// den Channel-Strip — getChannelIndex matcht panel->getIndex().
			for (auto* panel : mixer.getChannelStrips()) {
				if (panel != nullptr && panel->getIndex() == bus->getChannelIndex()) {
					panel->processMasterFX(*mb, transportPos, isPlayingNow, isRecordingNow);
					break;
				}
			}

			const float busVol = (! busMeta->mute) ? (float) busMeta->volume : 0.0f;
			if (busVol <= 0.0f) continue;
			const double busPan = busMeta->pan;
			const float busGainL = fastCos((M_PI * (busPan + 1) / 4));
			const float busGainR = fastSin((M_PI * (busPan + 1) / 4));

			const float* mL = mb->getReadPointer(0);
			const float* mR = mb->getReadPointer(1);

			if (outNumChans > 0 && outputData[0] && mL)
				juce::FloatVectorOperations::addWithMultiply(outputData[0], mL, busGainL * busVol, numSamples);
			if (outNumChans > 1 && outputData[1] && mR)
				juce::FloatVectorOperations::addWithMultiply(outputData[1], mR, busGainR * busVol, numSamples);

			// VU-Magnitude fuer den Bus-Strip aktualisieren.
			busMeta->magnitudeLeft  = mb->getMagnitude(0, 0, numSamples) * busVol * busGainL;
			busMeta->magnitudeRight = mb->getMagnitude(1, 0, numSamples) * busVol * busGainR;
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
			juce::FloatVectorOperations::addWithMultiply(outputData[hwIndex], src, gain, numSamples);
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
			juce::FloatVectorOperations::add(destL, srcL, numSamples);
		}
		if (outNumChans > 1) {
			auto* destR = bufferToFill.buffer->getWritePointer(1, bufferToFill.startSample);
			auto* srcR = metronomeBuffer.getReadPointer(metronomeBuffer.getNumChannels() > 1 ? 1 : 0);
			juce::FloatVectorOperations::add(destR, srcR, numSamples);
		}
	}

	// Process master FX chain (before master fader).
	// Buffer auf das erste Stereo-Paar (HW 0/1) beschränken — Master-FX-Plugins
	// sind faktisch immer Stereo (Mastering, Analyzer, …). Würde man den
	// gesamten Multi-Out-Buffer (z.B. 6/8 Kanäle) durchschicken, clearen viele
	// Plugins die ungenutzten Kanäle gemäß ihres Bus-Layouts — Synths, die auf
	// höhere OUT-Channel-Paare gerouted sind, werden dann stumm, obwohl ihre
	// Pegelanzeigen weiter ausschlagen (Pegel werden vor der FX-Schleife
	// berechnet).
	if (mixerPanel) {
		auto* masterPan = mixerPanel->getMasterPanel();
		if (masterPan) {
			const int masterFxChans = std::min(outNumChans, 2);
			if (masterFxChans > 0) {
				juce::AudioBuffer<float> masterBuf(outputData, masterFxChans, numSamples);
				const juce::int64 transportPos = navigator != nullptr ? navigator->getSamplePosition() : 0;
				const bool isPlaying  = navigator != nullptr && navigator->isPlaying();
				const bool isRecording = navigator != nullptr && navigator->isRecording();
				masterPan->processMasterFX(masterBuf, transportPos, isPlaying, isRecording);
			}
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
				juce::FloatVectorOperations::multiply(outputData[ch], gain, numSamples);
			}

			auto absMax = [](const float* d, int n) {
				juce::Range<float> r = juce::FloatVectorOperations::findMinAndMax(d, n);
				return std::max(std::abs(r.getStart()), std::abs(r.getEnd()));
			};
			master->magnitudeLeft  = (outNumChans > 0 && outputData[0]) ? absMax(outputData[0], numSamples) : 0.0f;
			master->magnitudeRight = (outNumChans > 1 && outputData[1]) ? absMax(outputData[1], numSamples) : 0.0f;
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

		// Statusleiste am ganz unteren Rand, über die volle Fensterbreite.
		// Achtung: layOutComponents weiter unten wird mit `r.getY() + 25` und
		// `r.getHeight()` aufgerufen — der editorView ragt also effektiv 25 px
		// unter r.getBottom() hinaus. Wir müssen also statusH + 25 reservieren,
		// damit der Editor die Statusleiste nicht überdeckt.
		const int statusH = 22;
		if (statusBar != nullptr) {
			statusBar->setBounds(0, getHeight() - statusH, getWidth(), statusH);
			r.removeFromBottom(statusH + 25);
			// Sicherstellen, dass die Statusleiste über allen anderen Children
			// liegt — sonst könnte ein nachträglich hinzugefügtes Panel sie
			// verdecken.
			statusBar->toFront(false);
		}

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
		menu.addItem(15, "Import Stems", true, false, nullptr);
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
		menu.addItem(206, "Log", true, false, nullptr);
		menu.addItem(203, "Level Meters", true, false, nullptr);
		menu.addItem(204, "Input Spectrum", true, false, nullptr);
		menu.addSeparator();
		menu.addItem(205, "Quickstart", true, false, nullptr);
		menu.addItem(207, "JavaScript Editor", true, false, nullptr);
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
		// Region-weite Edits — funktionieren auf der aktuellen Selektion und
		// spiegeln die Cmd-Shortcuts (X / C / V / D / T / B) im Track-View.
		menu.addItem(71, "Cut", true, false, nullptr);
		menu.addItem(72, "Copy", true, false, nullptr);
		menu.addItem(73, "Paste at marker", true, false, nullptr);
		menu.addItem(43, "Duplicate", true, false, nullptr);
		menu.addItem(44, "Split at marker", true, false, nullptr);
		menu.addItem(49, "Consolidate", true, false, nullptr);
		menu.addItem(70, "Bounce in Place", true, false, nullptr);
		menu.addItem(45, "Delete", true, false, nullptr);
		menu.addSeparator();

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

		// MIDI-spezifische Funktionen analog zum Audio-Submenu — Quantize ist
		// nur fuer MIDI-Regionen sinnvoll, und die beiden Editor-Fenster
		// (Piano Roll + Event Editor) gibt's bisher nur ueber Tastenkuerzel.
		PopupMenu midiMenu = PopupMenu();
		midiMenu.addItem(74, "Open Piano Roll", true, false, nullptr);
		midiMenu.addItem(75, "Open Message Editor", true, false, nullptr);
		midiMenu.addItem(41, "Quantize", true, false, nullptr);
		menu.addSubMenu("MIDI", midiMenu);
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
	else if (menuItemID == 15) {
		FileChooser chooser("Select a folder containing stems...");
		if (chooser.browseForDirectory()) {
			File folder = chooser.getResult();

			if (editorView->getNavigator() == nullptr) {
				editorView->OpenTrackView();
				editorView->getNavigator()->addChangeListener(this);
			}

			Array<File> wavFiles;
			folder.findChildFiles(wavFiles, File::findFiles, false, "*.wav");
			wavFiles.sort();

			const double sr = Project::getInstance()->getSampleRate();

			for (const File& file : wavFiles) {
				Track* t = TrackUtil::createAudioTrack(editor, editorView->getNavigator(), editor->getModule());
				if (t != nullptr) {
					t->setName(file.getFileNameWithoutExtension());
					t->addRegion(file.getFileNameWithoutExtension(), file, sr);
					Project::getInstance()->addAudioFile(file.getFileNameWithoutExtension(), file.getFullPathName());
				}
			}

			editorView->getNavigator()->adjustHeight();
			editorView->getNavigator()->repaint();
			editor->repaint();
		}
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
	else if (menuItemID == 205) {
		openQuickstart();
	}
	else if (menuItemID == 206) {
		editorView->openLog();
	}
	else if (menuItemID == 207) {
		openJavaScriptEditor();
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
				audioRegion->markBufferMutated();
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
			Track* t = navigator->getCurrentTrack();
			// Mehrere selektierte AudioRegions auf demselben Track? Dann erst
			// mergen — das Ergebnis ist per Definition schon konsolidiert
			// (sourceStartSample=0, Visible-Length == Buffer-Length). Wenn
			// nur eine Region selektiert ist, faellt der Pfad auf das
			// klassische region->consolidate() zurueck.
			if (t->mergeSelectedRegions()) {
				// Audio: Merge-Resultat ist per Definition schon konsolidiert
				// (sourceStartSample=0, Visible-Length == Buffer-Length).
				// MIDI: ebenso, da addMidiRegion frisch leer mit sss=0 startet.
			}
			else if (Region* region = t->getCurrentRegion()) {
				region->consolidate();
			}
		}
	}
	else if (menuItemID == 60) {
		if (navigator != nullptr && navigator->getCurrentTrack() != nullptr) {
			auto* action = new TrackDeleteAction(navigator, editor);
			Project::getInstance()->getUndoManager()->beginNewTransaction();
			Project::getInstance()->getUndoManager()->perform(action);
		}
	}
	// Region menu — entries spiegeln die im TrackNavigator schon verkabelten
	// Cmd-Shortcuts. Die Logik liegt jeweils in TrackNavigator/Track, der
	// Menupfad ruft sie nur auf.
	else if (menuItemID == 70) { // Bounce in Place
		if (navigator != nullptr)
			navigator->bounceSelectedRegionsInPlace();
	}
	else if (menuItemID == 71) { // Cut
		if (navigator != nullptr) {
			Session::getInstance()->clearRegionsClipboard();
			for (auto* t : navigator->getTracks())
				if (t != nullptr) t->copySelectedRegions();
			for (auto* t : navigator->getTracks())
				if (t != nullptr) t->removeSelectedRegions(true);
		}
	}
	else if (menuItemID == 72) { // Copy
		if (navigator != nullptr) {
			Session::getInstance()->clearRegionsClipboard();
			for (auto* t : navigator->getTracks())
				if (t != nullptr) t->copySelectedRegions();
		}
	}
	else if (menuItemID == 73) { // Paste at marker
		if (navigator != nullptr) {
			Track* dest = navigator->getCurrentTrack();
			if (dest != nullptr)
				dest->pasteRegions((long) navigator->getSamplePosition());
		}
	}
	else if (menuItemID == 74) { // Open Piano Roll (MIDI)
		if (navigator != nullptr && navigator->getCurrentTrack() != nullptr) {
			if (auto* mr = dynamic_cast<MidiRegion*>(navigator->getCurrentTrack()->getCurrentRegion()))
				navigator->openPianoRoll(mr);
		}
	}
	else if (menuItemID == 75) { // Open Message Editor (MIDI)
		if (navigator != nullptr && navigator->getCurrentTrack() != nullptr) {
			if (auto* mr = dynamic_cast<MidiRegion*>(navigator->getCurrentTrack()->getCurrentRegion()))
				navigator->openEventEditor(mr);
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
			restoreNavigatorAfterOpen();
		}
	}
	else if (menuItemID >= 1000) {
		if (!editor->confirmDiscardIfDirty()) return;
		String xmlData = sampleData.getReference(menuItemID - 1000);
		editor->setRunning(false);
		editor->cleanUp();
		editor->newFile(/*askIfDirty=*/ false);
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

void MainComponent::restoreNavigatorAfterOpen() {
	// Deferred via callAfterDelay damit pendendes Layout / Region-Thumbnail
	// aus openTracks() noch durchlaufen kann — sonst kommt der Navigator
	// gelegentlich leer oder mis-sized hoch.
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

void MainComponent::openQuickstart() {
	if (editorView == nullptr)
		return;

	auto* tabs = editorView->getEditorTab();
	if (tabs == nullptr)
		return;

	// Already open? Just bring it to front.
	for (int i = 0; i < tabs->getNumTabs(); ++i) {
		if (tabs->getTabNames()[i] == "Quickstart") {
			tabs->setCurrentTabIndex(i);
			return;
		}
	}

	if (quickstart == nullptr) {
		quickstart = std::make_unique<QuickstartComponent>();

		quickstart->setOnConfigureAudio([this]() {
			openSettings();
		});

#if defined(JUCE_PLUGINHOST_AU) || defined(JUCE_PLUGINHOST_VST)
		quickstart->setOnScanPlugins([]() {
			PluginManager::getInstance()->scanPlugins();
		});
#else
		quickstart->setOnScanPlugins([]() {});
#endif

		quickstart->setOnCreateInstrumentTrack([this]() {
			if (editorView->getNavigator() == nullptr) {
				editorView->OpenTrackView();
				editorView->getNavigator()->addChangeListener(this);
			}
			editor->addVirtualInstrumentTrack();
			editor->repaint();
		});

		quickstart->setOnOpenExistingTrack([this]() {
			// Gleicher Pfad wie der "Open"-Toolbar-Button.
			editor->setRunning(false);
			editor->openFile();
			editor->setRunning(true);
			restoreNavigatorAfterOpen();
		});

		quickstart->setRecentFilesProvider([]() {
			// Project laedt die Liste lazy — sicherstellen, dass sie frisch
			// vom Disk gelesen ist, bevor wir sie in den UI rendern.
			Project::getInstance()->loadRecentFileList();
			return Project::getInstance()->getRecentFiles();
		});

		quickstart->setOnOpenRecentFile([this](const juce::String& path) {
			juce::File f (path);
			if (! f.existsAsFile()) return;
			editor->setRunning(false);
			editor->openFile(f);
			editor->setRunning(true);
			restoreNavigatorAfterOpen();
		});

		quickstart->setAudioConfiguredProvider([this]() {
			return deviceManager.getCurrentAudioDevice() != nullptr
				&& deviceManager.getCurrentAudioDevice()->isOpen();
		});

#if defined(JUCE_PLUGINHOST_AU) || defined(JUCE_PLUGINHOST_VST)
		quickstart->setPluginsScannedProvider([]() {
			return PluginManager::getInstance()->getPlugins().size() > 0;
		});
#else
		quickstart->setPluginsScannedProvider([]() { return false; });
#endif

		quickstart->setInstrumentTrackProvider([this]() {
			auto* nav = editorView->getNavigator();
			if (nav == nullptr) return false;
			for (auto* t : nav->getTracks()) {
				if (t != nullptr && t->getType() == Track::INSTRUMENT)
					return true;
			}
			return false;
		});

		quickstart->setExistingTrackOpenedProvider([]() {
			// Done sobald ein Projekt mit bekanntem Pfad aktiv ist (also nicht
			// ein frisches "New File"). Project::isNewFile() ist true bis zum
			// ersten Save / Load.
			return ! Project::getInstance()->isNewFile();
		});
	}

	tabs->addTab("Quickstart", Colour(0xff1b1b25), quickstart.get(), false);
	tabs->setCurrentTabIndex(tabs->getNumTabs() - 1);
}

void MainComponent::openJavaScriptEditor() {
	if (editorView == nullptr)
		return;

	auto* tabs = editorView->getEditorTab();
	if (tabs == nullptr)
		return;

	// Already open? Just bring it to front.
	for (int i = 0; i < tabs->getNumTabs(); ++i) {
		if (tabs->getTabNames()[i] == "JavaScript") {
			tabs->setCurrentTabIndex(i);
			return;
		}
	}

	if (jsEditor == nullptr)
		jsEditor = std::make_unique<JavaScriptEditorComponent>();

	tabs->addTab("JavaScript", Colour(0xff1b1b25), jsEditor.get(), false);
	tabs->setCurrentTabIndex(tabs->getNumTabs() - 1);
}

void MainComponent::openSettings() {

	DialogWindow* window = AudioSettingsDialog::show(deviceManager,
		Project::getInstance()->getLookAndFeel());

	std::function<void(int)> lambda =
		[this](int /*result*/) {
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

	// Computer-Tastatur als MIDI-Input: jede gemappte Taste loest Note-On/Off
	// auf dem MIDI-Kanal des aktuell selektierten Tracks aus. Geht ueber den
	// Queue-Pfad des MidiProcessors (handleIncomingMidiMessage) — derselbe
	// Pfad wie externe MIDI-Hardware, also automatisch korrekt geroutet (der
	// Processor filtert via activeChannel, was an currentTrack haengt — siehe
	// MainComponent::changeListenerCallback).
	if (midiProcessor == nullptr || navigator == nullptr) return false;

	Track* current = navigator->getCurrentTrack();
	const int channel = (current != nullptr) ? current->getMidiChannel() : 1;

	for (auto& kv : keyCodeMidiNote) {
		// JUCE's KeyPress::isKeyCurrentlyDown erwartet fuer Buchstaben
		// uppercase ASCII (z.B. 'A'=65). Unsere Map ist mit lowercase
		// gefuellt, also umrechnen.
		const int keyCode = juce::CharacterFunctions::toUpperCase((juce_wchar) kv.first);
		const int note    = kv.second + 12 * currentOctave;
		if (note < 0 || note > 127) continue;

		if (KeyPress::isKeyCurrentlyDown(keyCode)) {
			if (!keyStates[note]) {
				midiProcessor->handleIncomingMidiMessage(
					nullptr,
					MidiMessage::noteOn(channel, note, (juce::uint8) 100));
				keyStates[note] = true;
			}
		}
		else {
			if (keyStates[note]) {
				midiProcessor->handleIncomingMidiMessage(
					nullptr,
					MidiMessage::noteOff(channel, note));
				keyStates[note] = false;
			}
		}
	}
	return true;
}

void MainComponent::modifierKeysChanged(const juce::ModifierKeys& modifiers) {
	isOptionPressed = modifiers.isAltDown();
}

bool MainComponent::keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) {
	// Globaler Start/Stop-Shortcut. TrackNavigator::keyPressed greift nur,
	// wenn der Navigator den Fokus hat — sobald irgendein anderes Panel
	// (Property-Editor-Canvas, Mixer, leerer Tab usw.) den Fokus hat, kommt
	// der Key hier oben am MainComponent an. TextEditor-Felder konsumieren
	// Space selbst (Eingabe), also wird das Toggle nicht beim Tippen
	// ausgeloest.
	if (key == juce::KeyPress::spaceKey) {
		if (editorView != nullptr) {
			if (auto* nav = editorView->getNavigator())
				nav->setPlaying(! nav->isPlaying());
		}
		return true;
	}

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
			// Same crash pattern as the File > New command path: cleanUp()
			// fired before any confirmation dialog. Confirm up front.
			if (!editor->confirmDiscardIfDirty()) return;

			editor->setRunning(false);
			Project::getInstance()->setNewFile(true);
			navigator->clearTracks();
			editor->cleanUp();
			editor->newFile(/*askIfDirty=*/ false);
			// Wie beim ersten App-Start: 2 Default-Audio-Tracks anlegen, damit
			// das frische Projekt sofort dropp- und bespielbar ist.
			navigator->ensureDefaultAudioTracks(2);
			editor->setRunning(true);
		}
		else if (tb->getItemId() == toolbarFactory->doc_save) {
			editor->saveFile();
		}
		else if (tb->getItemId() == toolbarFactory->doc_open) {
			editor->setRunning(false);
			editor->openFile();
			editor->setRunning(true);
			restoreNavigatorAfterOpen();
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

	// Beim Offline-Bounce wird setRunning(false) gesetzt, um den Realtime-
	// Audio-Callback ruhigzustellen — der Callback selbst checkt allerdings
	// schon isBouncing() und kehrt sowieso frueh zurueck. Ohne die zusaetzliche
	// isBouncing()-Bedingung hier wuerde BounceThread::run() processModule()
	// aufrufen und sofort an dieser Stelle aussteigen, womit die komplette
	// Signal-Kette zwischen TrackIn und AudioOut/AuxOut im Bounce stumm bleibt
	// (Aux-Sends, Insert-FX, alle Zwischenmodule).
	if (m == nullptr || (!isRunning() && !isBouncing())) return;

	m->process();

	// Process children in three passes so polyphonic VALUE data (and audio)
	// flows through TerminalModules without a one-buffer delay:
	//   1. IN terminals  — pull external data into the group
	//   2. Content       — read internal pins, run audio
	//   3. OUT terminals — push internal results to the external pin
	// "Create Module from Selection" appends terminals after content in the
	// children vector, so without this ordering the consumer (e.g. the
	// Sawtooth) reads the IN terminal *before* it has been refreshed.
	//
	// We also push the parent's audio settings down to every child here.
	// Without this, modules created at runtime (terminals from
	// createModuleFromSelection, duplicates) never receive a buffersize and
	// their per-block sample loops collapse to a no-op — silent groups.
	auto& children = *m->getModules();

	const double parentSr = m->getSampleRate();
	const int    parentBs = m->getBufferSize();

	auto inheritAudio = [&](Module* c) {
		if (c == nullptr) return;
		if (c->getSampleRate() != parentSr) c->setSampleRate((float)parentSr);
		if (c->getBufferSize() != parentBs) c->setBuffersize(parentBs);
	};

	for (auto* c : children) {
		auto* term = dynamic_cast<TerminalModule*>(c);
		if (term != nullptr && term->getDirection() == TerminalModule::Direction::IN) {
			inheritAudio(c);
			processModule(c);
		}
	}

	for (auto* c : children) {
		if (c == nullptr) continue;
		if (dynamic_cast<TerminalModule*>(c) != nullptr) continue;
		inheritAudio(c);
		processModule(c);
	}

	for (auto* c : children) {
		auto* term = dynamic_cast<TerminalModule*>(c);
		if (term != nullptr && term->getDirection() == TerminalModule::Direction::OUT) {
			inheritAudio(c);
			processModule(c);
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
		// dontSendNotification: wir spiegeln nur den geladenen Layer in die UI.
		// Default-async loest sonst comboBoxChanged -> editor->setCurrentLayer
		// erst irgendwann spaeter aus der Message-Queue aus. Wenn der Nutzer
		// in der Zwischenzeit das naechste Projekt oeffnet, feuert das Event
		// waehrend openFile B (z.B. im modalen Dirty-AlertWindow) — root ist
		// dort nullptr und rootLevelProxies haelt dangling Module* aus A.
		layerCombobox->getComboBox().setSelectedId(editor->getCurrentLayer() + 1,
		                                            juce::NotificationType::dontSendNotification);
}

void MainComponent::dirtyChanged(bool dirty) {

}

void MainComponent::startBounce(const juce::File& outputFile, double durationInSeconds, bool exportMp3, bool exportOgg)
{
	bounceProgress = 0.0;

	bounceThread = std::make_unique<BounceThread>(
		this, outputFile, durationInSeconds,
		[this](float p) { bounceProgress.store(p); },
		exportMp3, exportOgg
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
		startBounce(dialog->getOutputFile(), duration, dialog->getExportMp3(), dialog->getExportOgg());

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
