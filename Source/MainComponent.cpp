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
#include "ShellConsoleComponent.h"
#include "LogView.h"
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
#include "Licensing/LicenseManager.h"
#include "Settings.h"
#include "ModuleUtils.h"
#include "MidiClock.h"
#include "MidiRecorderModule.h"
#include "TrackUtil.h"
#include "Tracks/BounceThread.h"
#include "Tracks/Track.h"
#include "Tracks/TrackNavigator.h"
#include "Tracks/TimeLine.h"
#include "Tracks/TimelineMarker.h"
#include "PianoRollEditor.h"
#include "MidiRegionEventEditor.h"
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
#include "Actions/AddTrackAction.h"

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

// ===========================================================================
// Auto Arrange — baut aus dem Loop-Bereich automatisch eine Songstruktur.
//
// Modell (nicht-destruktiv): Der Loop-Bereich ist EIN "Block". Block 0 sind
// die bereits im Loop liegenden Originalregionen — die bleiben unangetastet.
// Pro Section/Wiederholung wird ein weiterer Block als Kopie hinter den
// vorigen gelegt; blockLen == exakte Loop-Laenge, damit alles nahtlos
// aneinanderschliesst. Optional werden Section-Marker auf der Timeline
// gesetzt. Undo nimmt genau die erzeugten Kopien + Marker zurueck.
// ===========================================================================
namespace {

struct ArrangeSection {
	juce::String name;
	int          repeats = 0;   // Anzahl Loop-Bloecke dieser Section
};

class AutoArrangeAction : public juce::UndoableAction {
public:
	AutoArrangeAction(TrackNavigator* nav,
	                  long loopStartSamples,
	                  long blockLenSamples,
	                  std::vector<ArrangeSection> sectionsIn,
	                  bool placeMarkersIn,
	                  double sampleRateIn)
		: navigator(nav),
		  loopStart(loopStartSamples),
		  blockLen(blockLenSamples),
		  sections(std::move(sectionsIn)),
		  placeMarkers(placeMarkersIn),
		  sampleRate(sampleRateIn)
	{}

	bool perform() override
	{
		createdRegions.clear();
		createdMarkers.clear();

		if (navigator == nullptr || blockLen <= 0 || sampleRate <= 0.0)
			return false;

		const long loopEnd = loopStart + blockLen;

		// Teilnehmende Tracks + ihre Quell-Regionen (Block 0). Nur Tracks mit
		// Inhalt im Loop-Bereich werden arrangiert; leere Tracks bleiben leer.
		struct TrackSources { Track* track; std::vector<Region*> sources; };
		std::vector<TrackSources> participating;

		for (Track* t : navigator->getTracks()) {
			if (t == nullptr) continue;
			std::vector<Region*> src;
			for (Region* r : t->getRegions()) {
				if (r == nullptr) continue;
				const long off = (long) r->getSampleOffset();
				const long end = off + (long) r->getNumSamples();
				if (off < loopEnd && end > loopStart)   // ueberlappt den Loop
					src.push_back(r);
			}
			if (!src.empty())
				participating.push_back({ t, std::move(src) });
		}

		int totalBlocks = 0;
		for (const auto& s : sections)
			totalBlocks += juce::jmax(0, s.repeats);
		if (totalBlocks <= 0)
			return false;

		// Bloecke 1..totalBlocks-1 als Kopien anlegen (Block 0 == Originale).
		for (int i = 1; i < totalBlocks; ++i) {
			const long blockBase = loopStart + (long) i * blockLen;
			for (auto& ts : participating) {
				for (Region* src : ts.sources) {
					const long rel = juce::jmax((long) 0,
					                            (long) src->getSampleOffset() - loopStart);
					Region* placed = ts.track->pasteRegion(src, blockBase + rel);
					if (placed != nullptr)
						createdRegions.push_back({ ts.track, placed });
				}
			}
		}

		// Section-Marker setzen (an jeder Section-Grenze, inkl. Block 0).
		if (placeMarkers && navigator->getTimeLine() != nullptr) {
			long cumBlocks = 0;
			for (const auto& s : sections) {
				if (s.repeats <= 0) continue;
				const double pos = (double) (loopStart + cumBlocks * blockLen) / sampleRate;
				TimelineMarker m;
				m.position = pos;
				m.name     = s.name;
				m.type     = TimelineMarker::Type::Normal;
				navigator->getTimeLine()->addMarker(m);
				createdMarkers.push_back(pos);
				cumBlocks += s.repeats;
			}
		}

		refresh();
		return true;
	}

	bool undo() override
	{
		for (auto& cr : createdRegions)
			if (cr.track != nullptr && cr.region != nullptr)
				cr.track->removeRegion(cr.region, true);
		createdRegions.clear();

		if (navigator != nullptr && navigator->getTimeLine() != nullptr) {
			auto* tl = navigator->getTimeLine();
			for (double pos : createdMarkers) {
				const auto& mk = tl->getMarkers();
				for (int idx = 0; idx < (int) mk.size(); ++idx) {
					if (std::abs(mk[idx].position - pos) < 1e-6) {
						tl->removeMarker(idx);
						break;
					}
				}
			}
		}
		createdMarkers.clear();

		refresh();
		return true;
	}

	juce::String getDescription() const { return "Auto Arrange"; }

private:
	void refresh()
	{
		if (navigator == nullptr) return;
		navigator->adjustHeight();
		navigator->repaint();
		navigator->sendChangeMessage();
	}

	struct Created { Track* track = nullptr; Region* region = nullptr; };

	TrackNavigator*             navigator   = nullptr;
	long                        loopStart   = 0;
	long                        blockLen    = 0;
	std::vector<ArrangeSection> sections;
	bool                        placeMarkers = true;
	double                      sampleRate   = 0.0;

	std::vector<Created> createdRegions;
	std::vector<double>  createdMarkers;
};

// Modaler Dialog: editierbare Section-Liste (Name + Anzahl Loop-Bloecke),
// Marker-Toggle und Live-Zusammenfassung. Liefert die konfigurierten Sections
// per Callback zurueck.
class AutoArrangeDialog : public juce::Component {
public:
	AutoArrangeDialog(double blockSeconds, double blockBars)
		: blockSeconds(blockSeconds), blockBars(blockBars)
	{
		header.setText("Auto Arrange — Songstruktur aus dem Loop-Bereich",
		               juce::dontSendNotification);
		header.setFont(juce::Font(16.0f, juce::Font::bold));
		addAndMakeVisible(header);

		blockInfo.setJustificationType(juce::Justification::centredLeft);
		blockInfo.setText(juce::String("Loop-Block: ")
		                  + juce::String(blockBars, 2) + " Takte  ("
		                  + juce::String(blockSeconds, 2) + " s)",
		                  juce::dontSendNotification);
		addAndMakeVisible(blockInfo);

		colName.setText("Section", juce::dontSendNotification);
		colName.setFont(juce::Font(12.0f, juce::Font::bold));
		colReps.setText("x Loop", juce::dontSendNotification);
		colReps.setFont(juce::Font(12.0f, juce::Font::bold));
		addAndMakeVisible(colName);
		addAndMakeVisible(colReps);

		// Default-Popstruktur (genau kNumRows Eintraege). repeats == Anzahl
		// Loop-Bloecke der Section.
		const std::pair<const char*, int> defaults[kNumRows] = {
			{ "Intro",  1 }, { "Verse",  2 }, { "Chorus", 2 }, { "Verse",  2 },
			{ "Chorus", 2 }, { "Bridge", 1 }, { "Chorus", 2 }, { "Outro",  1 },
		};

		for (int i = 0; i < kNumRows; ++i) {
			auto name = std::make_unique<juce::TextEditor>();
			auto reps = std::make_unique<juce::TextEditor>();
			name->setText(defaults[i].first, juce::dontSendNotification);
			reps->setText(juce::String(defaults[i].second), juce::dontSendNotification);
			reps->setInputRestrictions(3, "0123456789");
			name->onTextChange = [this]{ updateSummary(); };
			reps->onTextChange = [this]{ updateSummary(); };
			addAndMakeVisible(*name);
			addAndMakeVisible(*reps);
			nameEditors.push_back(std::move(name));
			repsEditors.push_back(std::move(reps));
		}

		markersToggle.setButtonText("Section-Marker auf der Timeline setzen");
		markersToggle.setToggleState(true, juce::dontSendNotification);
		addAndMakeVisible(markersToggle);

		summary.setJustificationType(juce::Justification::centredLeft);
		addAndMakeVisible(summary);

		arrangeButton.setButtonText("Arrange");
		cancelButton.setButtonText("Cancel");
		arrangeButton.onClick = [this]{ if (onArrange) onArrange(); };
		cancelButton.onClick  = [this]{ if (onCancel)  onCancel();  };
		addAndMakeVisible(arrangeButton);
		addAndMakeVisible(cancelButton);

		updateSummary();
		setSize(420, 150 + kNumRows * 30 + 90);
	}

	void paint(juce::Graphics& g) override { g.fillAll(juce::Colour(0xff2b2b2b)); }

	void resized() override
	{
		auto r = getLocalBounds().reduced(14);
		header.setBounds(r.removeFromTop(24));
		r.removeFromTop(4);
		blockInfo.setBounds(r.removeFromTop(20));
		r.removeFromTop(8);

		auto headerRow = r.removeFromTop(18);
		colReps.setBounds(headerRow.removeFromRight(70));
		colName.setBounds(headerRow);

		for (int i = 0; i < kNumRows; ++i) {
			auto row = r.removeFromTop(28).reduced(0, 2);
			repsEditors[i]->setBounds(row.removeFromRight(70));
			row.removeFromRight(8);
			nameEditors[i]->setBounds(row);
		}

		r.removeFromTop(6);
		markersToggle.setBounds(r.removeFromTop(24));
		r.removeFromTop(4);
		summary.setBounds(r.removeFromTop(22));
		r.removeFromTop(8);

		auto buttons = r.removeFromTop(30);
		arrangeButton.setBounds(buttons.removeFromRight(110));
		buttons.removeFromRight(8);
		cancelButton.setBounds(buttons.removeFromRight(110));
	}

	std::vector<ArrangeSection> getSections() const
	{
		std::vector<ArrangeSection> out;
		for (int i = 0; i < kNumRows; ++i) {
			const juce::String nm = nameEditors[i]->getText().trim();
			const int reps = repsEditors[i]->getText().getIntValue();
			if (nm.isNotEmpty() && reps > 0)
				out.push_back({ nm, reps });
		}
		return out;
	}

	bool getPlaceMarkers() const { return markersToggle.getToggleState(); }

	std::function<void()> onArrange;
	std::function<void()> onCancel;

private:
	void updateSummary()
	{
		int blocks = 0;
		for (int i = 0; i < kNumRows; ++i) {
			if (nameEditors[i]->getText().trim().isNotEmpty())
				blocks += juce::jmax(0, repsEditors[i]->getText().getIntValue());
		}
		const double totalSec = blocks * blockSeconds;
		summary.setText("Gesamt: " + juce::String(blocks) + " Bloecke  ~ "
		                + juce::String((int) (totalSec / 60.0)) + ":"
		                + juce::String((int) std::fmod(totalSec, 60.0)).paddedLeft('0', 2)
		                + " min", juce::dontSendNotification);
	}

	static constexpr int kNumRows = 8;

	double blockSeconds = 0.0;
	double blockBars    = 0.0;

	juce::Label header, blockInfo, colName, colReps, summary;
	std::vector<std::unique_ptr<juce::TextEditor>> nameEditors;
	std::vector<std::unique_ptr<juce::TextEditor>> repsEditors;
	juce::ToggleButton markersToggle;
	juce::TextButton   arrangeButton, cancelButton;
};

} // namespace

namespace {
    // Wrapper-Komponente fuer ein shared_ptr<MixerPanel>: haelt eine eigene
    // shared_ptr-Kopie (Lifetime-Anker) + ein Viewport drumherum. Damit kann
    // der MixerPanel-Inhalt in einem DockablePanel landen, ohne dass die
    // SynthEditor-eigene shared_ptr<MixerPanel> entkoppelt werden muss.
    class MixerDockContent : public juce::Component
    {
    public:
        explicit MixerDockContent (std::shared_ptr<MixerPanel> mp)
            : mixerPanel (std::move (mp))
        {
            if (mixerPanel != nullptr)
            {
                viewport.setViewedComponent (mixerPanel.get(), false);
                viewport.setScrollBarsShown (true, true);
                viewport.setScrollOnDragEnabled (false);
                viewport.setWantsKeyboardFocus (false);
                viewport.setMouseClickGrabsKeyboardFocus (false);
                addAndMakeVisible (viewport);
            }
        }
        void resized() override { viewport.setBounds (getLocalBounds()); }
    private:
        juce::Viewport viewport;
        std::shared_ptr<MixerPanel> mixerPanel;
    };

    // Holder mit Audio-Thread-Atomic-Ref-Lifecycle: bei Construction wird der
    // frische Pointer in den Atomic geschrieben, bei Destruction wieder
    // genullt. Damit kann das DockablePanel mid-session geschlossen / neu
    // erzeugt werden, ohne dass getNextAudioBlock auf einen freed pointer
    // referenziert.
    template <typename T>
    class AudioRefHolder : public juce::Component
    {
    public:
        AudioRefHolder (std::unique_ptr<T> owned, std::atomic<T*>& slot)
            : audioSlot (slot), content (std::move (owned))
        {
            if (content != nullptr)
            {
                addAndMakeVisible (*content);
                audioSlot.store (content.get(), std::memory_order_release);
            }
        }
        ~AudioRefHolder() override { audioSlot.store (nullptr, std::memory_order_release); }
        void resized() override { if (content) content->setBounds (getLocalBounds()); }
    private:
        std::atomic<T*>& audioSlot;
        std::unique_ptr<T> content;
    };

    // Tools-Container: haelt die MainTabbedComponent, die als
    // Project::setSupplemental(...) angemeldet ist. Bei Destruction
    // setSupplemental(nullptr), sonst baumeln Project-Pointer auf toten Tab.
    class ToolsDockContent : public juce::Component
    {
    public:
        explicit ToolsDockContent (std::unique_ptr<MainTabbedComponent> t)
            : tabs (std::move (t))
        {
            if (tabs != nullptr)
            {
                addAndMakeVisible (*tabs);
                Project::getInstance()->setSupplemental (tabs.get());
            }
        }
        ~ToolsDockContent() override
        {
            if (auto* proj = Project::getInstance())
                proj->setSupplemental (nullptr);
        }
        void resized() override { if (tabs) tabs->setBounds (getLocalBounds()); }
        MainTabbedComponent* getTabs() const noexcept { return tabs.get(); }
    private:
        std::unique_ptr<MainTabbedComponent> tabs;
    };
}

//==============================================================================
MainComponent::MainComponent()
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
	// Damit SynthEditor::keyPressed(KeyPress, Component*) auf dem Root-Editor
	// ueberhaupt feuert (Shift-A oeffnet Add-Menue, Ctrl/Cmd-A waehlt alle),
	// muss der Editor als KeyListener auf sich selbst haengen. Im Sub-Editor-
	// Pfad (SynthEditor::openSubModule) wird genau das auch gemacht — beim
	// Root-Editor stand das frueher im (mittlerweile aufgeloesten) EditorComponent
	// und ist beim Refactor verloren gegangen.
	editor->addKeyListener(editor);
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

	// Detach the MenuBarModel base class from the command manager BEFORE any
	// member destructor (which runs ApplicationCommandManager's dtor and frees
	// it) can leave a dangling reference. MenuBarModel destructs after data
	// members (it's the base class), and its own dtor calls
	// setApplicationCommandManagerToWatch(nullptr) → removeListener on a
	// freed commandManager.
	setApplicationCommandManagerToWatch(nullptr);

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
		// Aktuelles Dock-Layout persistieren, bevor wir alles abreissen.
		// Beim naechsten Start wird die Datei in createStudioLayout via
		// loadDockLayout() wieder eingespielt — d.h. der User findet seine
		// Performance-Anordnung der Panels nach Restart wieder vor.
		saveDockLayout();

		// Audio-Thread-Refs zuerst nullen, damit getNextAudioBlock waehrend
		// des Teardowns nicht in einen sterbenden LevelMeter/Spektrum greift.
		levelMeterAudio.store(nullptr, std::memory_order_release);
		spectrumAudio.store(nullptr, std::memory_order_release);

		// Globale Project-Pointer VOR dem DockManager nullen. Die Reihenfolge
		// matters, weil ~DockManager seine allPanels-OwnedArray vom Anfang weg
		// abbaut — d.h. PropertyView (Panel-Index 0) verschwindet zuerst,
		// Editor (Index 1) als naechstes. ~EditorComponent ruft ~SynthEditor
		// -> cleanUp(), das `Project::getPropertyView()->selectionChanged()`
		// aufruft (null-gecheckt). Ohne das setPropertyView(nullptr) hier
		// laeuft cleanUp() in einen Dangling-Pointer.
		if (auto* proj = Project::getInstance()) {
			proj->setJavaScriptEditor(nullptr);
			proj->setPropertyView(nullptr);
		}

		// Control-Surface haelt Raw-Pointer auf editor + Quell-Module ueber
		// shadow-bindings (juce::Value::referTo). VOR dockManager freigeben,
		// sonst zeigen die Bindings auf zerstoerte Slider und der Panel-Dtor
		// (referTo-Tear-down) liest tote ValueSources.
		controlSurfacePanel = nullptr;

		// SynthEditor::tab ist ein raw-Pointer auf editorAreaTab. dockManager
		// zerstoert gleich das Editor-DockablePanel und damit editorAreaTab;
		// danach laeuft beim Member-Reverse-Destruktur editorOwned -> ~Synth-
		// Editor -> cleanUp(), das tab->getNumTabs() ruft. Ohne setTab(nullptr)
		// derefferiert das freed memory. Frueher passierte das im
		// EditorComponent-Dtor; nach dessen Aufloesung muss MainComponent es
		// machen.
		if (editor != nullptr)
			editor->setTab(nullptr);

		// DockManager besitzt jetzt propertyView, editorAreaTab und jsEditor.
		// Sein Dtor cleart die OwnedArray<DockablePanel> -> kaskadiert in die
		// Content-Komponenten. Raw-Pointer hinterher null setzen, damit kein
		// Code-Pfad sie aus Versehen noch anfasst.
		dockManager.reset();
		propertiesDockPanel = nullptr;
		editorDockPanel     = nullptr;
		jsDockPanel         = nullptr;
		mixerDockPanel = nullptr;
		toolsDockPanel = nullptr;
		logDockPanel = nullptr;
		levelDockPanel = nullptr;
		spectrumDockPanel = nullptr;
		jsEditor       = nullptr;
		propertyView   = nullptr;
		editorAreaTab  = nullptr;
		toolsTab       = nullptr;

		// editorViewport/trackViewport/consoleComponent/editorOwned werden
		// von Member-Destruktion gegen Ende der Funktion abgeraumt. editor
		// (raw Pointer auf editorOwned.get()) bleibt formal noch gueltig bis
		// dahin — verwendete UI-Pfade sind aber schon abgehaengt.

		tab = nullptr;
		delete lockButton;
		menu = nullptr;
		// Buttons gehoeren der Toolbar — Pointer nullen, bevor wir das Owner-
		// Objekt zerstoeren, damit timerCallback() bei spaeterer Wiederbelebung
		// nicht auf befreite Buttons greift.
		playToolbarButton = nullptr;
		recordToolbarButton = nullptr;
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

	// Editor explizit VOR midiProcessor zerstoeren. Sonst feuert ~SynthEditor
	// im Member-Reverse-Destruktur-Phase setRunning(true) am Ende seiner Dtor-
	// Logik (siehe SynthEditor.cpp:126), die FastBus-publish triggert eine
	// MidiProcessor-Lambda, die auf einem dann schon ge'delete'eten MidiProcessor-
	// `this` startTimer aufruft -> EXC_BAD_ACCESS. Reihenfolge hier: erst Editor
	// (laesst FastBus-publish auf einen noch lebenden MidiProcessor laufen),
	// dann MidiProcessor (und FastBus-Subscription stirbt mit ihm via Funktor-
	// Destruktor).
	editorOwned.reset();

	// Navigator explizit VOR trackViewport zerstoeren. ~TrackNavigator ruft
	// `view->getHorizontalScrollBar().removeListener(this)` — `view` ist ein
	// roher Pointer auf trackViewport. Bei Member-Reverse-Destruktion ginge
	// trackViewport (Zeile spaeter im Header) erst, dann erst navigator
	// (shared_ptr-Member frueher) -> UAF im Scroll-Listener-Cleanup.
	navigator.reset();

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
		auto* item = toolbar->getItemComponent(i);
		item->addListener(this);

		if (layerCombobox == nullptr) {
			layerCombobox = dynamic_cast<ToolbarComboBox*>(item);
		}
		if (layerCombobox != nullptr) {

			layerCombobox->getComboBox().addListener(this);
		}

		if (auto* tb = dynamic_cast<juce::ToolbarButton*>(item)) {
			if (tb->getItemId() == toolbarFactory->app_play)
				playToolbarButton = dynamic_cast<TransportToolbarButton*>(tb);
			else if (tb->getItemId() == toolbarFactory->app_record)
				recordToolbarButton = dynamic_cast<TransportToolbarButton*>(tb);
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

	// Ownership der Hauptpanels wandert spaeter zum DockManager — hier
	// allozieren wir die Komponenten zunaechst auf dem Heap und halten den
	// Raw-Pointer fuer die Bestands-Verdrahtung. Erst nach dem Aufbau aller
	// Verbindungen werden sie via dock.createPanel an den DockManager
	// uebergeben.
	auto* propertyViewOwned = new PropertyView();
	propertyView = propertyViewOwned;
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

	// Editor + dessen TopTab direkt im MainComponent aufbauen (EditorComponent
	// wurde aufgeloest). DockablePanel "Editor" bekommt editorAreaTab als
	// Content; editorViewport haelt den SynthEditor (deleteWhenRemoved=false,
	// Lifetime ueber editorOwned). Reihenfolge analog zum alten EditorComponent-
	// Ctor, damit alle nachgelagerten Wirings (PropertyView, JS-Editor, etc.)
	// unveraendert greifen.
	editorOwned = std::make_unique<SynthEditor>(sampleRate, buffersize);
	editor = editorOwned.get();

	editorViewport = std::make_unique<juce::Viewport>();
	editorViewport->setSize(500, 200);
	editorViewport->setViewedComponent(editor, false);
	editorViewport->setScrollBarsShown(true, true);
	editorViewport->setScrollOnDragEnabled(false);
	editorViewport->setWantsKeyboardFocus(false);
	editorViewport->setMouseClickGrabsKeyboardFocus(false);

	editor->setWantsKeyboardFocus(true);
	editor->setMouseClickGrabsKeyboardFocus(true);

	auto editorAreaTabOwned = std::make_unique<MainTabbedComponent>();
	editorAreaTab = editorAreaTabOwned.get();
	editorAreaTab->addTab("Editor", juce::Colours::grey, editorViewport.get(), false);
	editorAreaTab->setInterceptsMouseClicks(true, true);

	// MixerPanel — wird in MainComponent gehalten (shared_ptr) und vom Mixer-
	// DockablePanel via MixerDockContent angezeigt. Erzeugung VOR dem
	// SynthEditor::setMixer-Aufruf, weil setMixer den Pointer aufnimmt.
	mixerPanel = std::make_shared<MixerPanel>();
	mixerPanel->setSynthEditor(editor);
	editor->setMixer(mixerPanel);

	// Navigator + Track-Tab anlegen — frueher passierte das am Ende des
	// EditorComponent-Ctors via OpenTrackView(). Wir rufen die jetzt am
	// MainComponent lebende openTrackView() explizit auf, damit nachfolgende
	// Wirings (`navigator->addChangeListener(this)` etc.) einen gueltigen
	// Pointer haben.
	openTrackView();

	navigator->addChangeListener(this);
	getTopLevelComponent()->addKeyListener(navigator.get());

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
			TrackUtil::createVirtualInstrumentTrack(args.at(1), pos, navigator.get(), editor);
			return "Instrument track added.";
		}
		if (args[0] == "midi") {
			TrackUtil::createMIDITrack(editor, navigator.get(), editor->getModule());
			return "MIDI track added.";
		}
		if (args[0] == "audio") {
			TrackUtil::createAudioTrack(editor, navigator.get(), editor->getModule());
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
	editor->addEditorListener(propertyView);
	propertyView->getBrowser()->addChangeListener(editor);
	// Hand the editor to PropertyView so its Instruments tab can spawn an
	// Instrument track on double-click without a plugin browser dialog.
	propertyView->setEditor(editor);

	Project::getInstance()->setPropertyView(propertyView);

	editor->setTab(editorAreaTab);
	editorAreaTab->addChangeListener(this);
	editor->addEditorListener(this);
	addMouseListener(propertyView->getBrowser(), false);

	commandManager->registerAllCommandsForTarget(editor);
	commandManager->setFirstCommandTarget(editor);

	// JavaScript editor as a right-hand pane. Created here so it is part of the
	// dock layout from the first resized() onwards; was previously instantiated
	// lazily into a tab inside editorView. Schliessbar — Wiederanzeige via Menue
	// (showOrCreateJavaScriptPanel).
	auto* jsEditorOwned = createJavaScriptEditorComponent();

	// Control-Surface-Panel: WIP, weiter deaktiviert (siehe historischer Kommentar
	// — Shadow-Tree neben dem Editor, per-Typ-Bindings + Auto-Rebuild offen,
	// Listener-/Lifetime-Pfade brauchen Hardening. Wenn stabil, bekommt es ein
	// eigenes DockablePanel statt eines festen Layout-Items.).
	constexpr bool kEnableControlSurfacePanel = false;
	if (kEnableControlSurfacePanel) {
		controlSurfacePanel = std::make_unique<ControlSurfacePanel>();
		controlSurfacePanel->setEditor(editor);
		addAndMakeVisible(*controlSurfacePanel);
	}

	// DockManager mit den drei Hauptpanels aufbauen. Ownership der gerade
	// allozierten Komponenten geht hier auf den Manager ueber; die Raw-Pointer
	// (propertyView, editorView, jsEditor) bleiben gueltig solange dockManager
	// existiert.
	dockManager = std::make_unique<docking::DockManager>();
	addAndMakeVisible(dockManager.get());

	// DockSide::Center wuerde das Panel in den bereits existierenden Tab
	// mergen — wir wollen drei separate Splits. Reihenfolge: Properties links
	// (erstes Panel landet als Solo unter root), Editor rechts (anhaengen),
	// JavaScript rechts (wieder anhaengen) → [Props | Editor | JS].
	propertiesDockPanel = dockManager->createPanel(
		"properties", "Properties",
		std::unique_ptr<juce::Component>(propertyViewOwned),
		docking::DockSide::Left);
	// "Editor" Panel content = editorAreaTab (MainTabbedComponent mit
	// Editor/Navigator/Quickstart Sub-Tabs). Ownership wandert zum
	// DockablePanel; der Raw-Pointer editorAreaTab bleibt gueltig.
	editorDockPanel = dockManager->createPanel(
		"editor", "Editor",
		std::unique_ptr<juce::Component>(editorAreaTabOwned.release()),
		docking::DockSide::Right);
	jsDockPanel = dockManager->createPanel(
		"javascript", "JavaScript",
		std::unique_ptr<juce::Component>(jsEditorOwned),
		docking::DockSide::Right);

	// Strukturelle Panels sollen sich nicht zu Tode schliessen lassen —
	// "Close" wuerde den DockManager mit verwaisten Wirings (Project::setX,
	// EditorListener, KeyListener) zuruecklassen. Floating/Re-Docking bleibt
	// unangetastet erlaubt.
	if (propertiesDockPanel != nullptr) propertiesDockPanel->setClosable(false);
	if (editorDockPanel     != nullptr) editorDockPanel    ->setClosable(false);

	// JS-Panel darf geschlossen werden — beim Schliessen die rohen Pointer +
	// Project-Wiring nullen, sonst dangling. Wiederanzeige ueber das Menue.
	if (jsDockPanel != nullptr)
	{
		jsDockPanel->setClosable(true);
		jsDockPanel->onClosed = [this]()
		{
			jsEditor    = nullptr;
			jsDockPanel = nullptr;
			if (auto* proj = Project::getInstance()) proj->setJavaScriptEditor(nullptr);
		};
	}

	// Default-Initialbreiten an das alte StretchableLayout angelehnt
	// (Editor ~60% der Breite, Properties ~20%, JS ~20%). DockSplitNode::resized
	// normalisiert das anhand der wirklich verfuegbaren Breite, also reichen
	// relative Pixel-Gewichte.
	if (auto* root = dockManager->getRootNode())
		if (root->getNumChildren() == 3)
		{
			root->setChildSize(0, 220); // Properties
			root->setChildSize(1, 720); // Editor
			root->setChildSize(2, 260); // JS
		}

	// Bottom-Row Panels: Mixer, Tools, Log, Level Meter, Spektrum. Sie sollen
	// per Default UNTER dem Editor liegen (vertikaler Split nur in der
	// Editor-Spalte), also via dockPanel(panel, editorTabNode, Bottom) erst
	// Mixer einhaengen — danach Tools/Log/Level/Spektrum als Tabs in einem
	// weiteren Bottom-Slot neben Mixer.
	auto* mixerOwned = new MixerDockContent(mixerPanel);
	mixerDockPanel = dockManager->createPanel(
		"mixer", "Mixer",
		std::unique_ptr<juce::Component>(mixerOwned),
		docking::DockSide::Center);
	if (mixerDockPanel != nullptr) mixerDockPanel->setClosable(false);

	auto toolsTabOwned = std::make_unique<MainTabbedComponent>();
	toolsTab = toolsTabOwned.get();
	auto* toolsHolderOwned = new ToolsDockContent(std::move(toolsTabOwned));
	toolsDockPanel = dockManager->createPanel(
		"tools", "Tools",
		std::unique_ptr<juce::Component>(toolsHolderOwned),
		docking::DockSide::Center);
	if (toolsDockPanel != nullptr) toolsDockPanel->setClosable(false);

	logDockPanel = dockManager->createPanel(
		"log", "Log",
		std::make_unique<LogView>(),
		docking::DockSide::Center);
	if (logDockPanel != nullptr) logDockPanel->setClosable(true);

	levelMeter = std::make_unique<AudioLevelMeter>();
	levelDockPanel = dockManager->createPanel(
		"level", "Level Meter",
		std::make_unique<AudioRefHolder<AudioLevelMeter>>(std::move(levelMeter), levelMeterAudio),
		docking::DockSide::Center);
	if (levelDockPanel != nullptr) levelDockPanel->setClosable(true);

	spectrumAnalyzer = std::make_unique<SpectrumAnalyzer>();
	spectrumAnalyzer->setNumberOfBars(128);
	spectrumDockPanel = dockManager->createPanel(
		"spectrum", "Spektrum",
		std::make_unique<AudioRefHolder<SpectrumAnalyzer>>(std::move(spectrumAnalyzer), spectrumAudio),
		docking::DockSide::Center);
	if (spectrumDockPanel != nullptr) spectrumDockPanel->setClosable(true);

	// Anordnung: alle fuenf sind via Center initial in den Editor-Tab gewandert
	// (firstTabNodeOrCreate-Verhalten). Jetzt explizit unter den Editor docken
	// (Bottom-Split) und untereinander tabifizieren.
	if (auto* editorTabNode = dynamic_cast<docking::DockTabNode*>(
			editorDockPanel != nullptr ? editorDockPanel->getParentComponent() : nullptr))
	{
		auto* mgr = dockManager.get();
		mgr->dockPanel(mixerDockPanel,    editorTabNode, docking::DockSide::Bottom);
		// mixerDockPanel hat jetzt sein eigenes TabNode (rechts/unter editor).
		if (auto* mixerTabNode = dynamic_cast<docking::DockTabNode*>(
				mixerDockPanel != nullptr ? mixerDockPanel->getParentComponent() : nullptr))
		{
			mgr->dockPanel(toolsDockPanel,    mixerTabNode, docking::DockSide::Right);
			if (auto* toolsTabNode = dynamic_cast<docking::DockTabNode*>(
					toolsDockPanel != nullptr ? toolsDockPanel->getParentComponent() : nullptr))
			{
				mgr->dockPanel(logDockPanel,      toolsTabNode, docking::DockSide::Center);
				mgr->dockPanel(levelDockPanel,    toolsTabNode, docking::DockSide::Center);
				mgr->dockPanel(spectrumDockPanel, toolsTabNode, docking::DockSide::Center);
			}
		}
	}

	// Aus User-Verzeichnis (.synthlabdock) wiederherstellen, falls vorhanden.
	loadDockLayout();

#if defined(JUCE_PLUGINHOST_AU) || defined(JUCE_PLUGINHOST_VST)
	pluginMenu = PluginManager::getInstance()->buildPluginMenu();
#endif
	editor->updateProject(File());
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

	// Transport-Button-Feedback: Play -> dauergrün, Record -> rotes Blinken im
	// aktuellen Tempo. State kommt vom Navigator, das Blinken läuft eigenständig
	// im Button-Timer (siehe TransportToolbarButton).
	if (navigator != nullptr) {
		const bool playing   = navigator->isPlaying();
		const bool recording = navigator->isRecording();

		if (playToolbarButton != nullptr) {
			playToolbarButton->setActiveMode(playing
				? TransportToolbarButton::Mode::Solid
				: TransportToolbarButton::Mode::Idle);
		}
		if (recordToolbarButton != nullptr) {
			recordToolbarButton->setBlinkBpm(Project::getInstance()->getTempo());
			recordToolbarButton->setActiveMode(recording
				? TransportToolbarButton::Mode::Blinking
				: TransportToolbarButton::Mode::Idle);
		}
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

	// Audio-Thread liest die Atomic-Refs, damit die UI-Seite das Panel
	// schliessen / re-konstruieren kann, ohne dass dieser Aufruf in einen
	// freed pointer fuehrt. Wenn das Panel zu ist, ist der Pointer einfach
	// nullptr und wir ueberspringen den Block.
	if (auto* lm = levelMeterAudio.load(std::memory_order_acquire))
		lm->processAudioBlock(bufferToFill);

	if (auto* sa = spectrumAudio.load(std::memory_order_acquire))
		sa->getNextAudioBlock(bufferToFill);

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

	// Try-lock against the main thread mutating the module graph (openFile,
	// loadModule, cleanUp etc.). If the main thread is currently holding the
	// lock for a load, skip processing this buffer rather than racing with
	// vector::push_back reallocations that would free the storage we'd be
	// iterating. The audio host fills the buffer with silence for one block
	// which is preferable to a use-after-free.
	const juce::ScopedTryLock graphLock(
		AudioManager::getInstance()->getProcessGraphLock());

	if (!graphLock.isLocked()) {
		// Lock held by another thread — main thread is loading. Bail out
		// cleanly so we don't touch the module tree mid-mutation.
		return;
	}

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

		metronome->generateBlock(metronomeBuffer, (long)navigator->getSamplePosition());

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

void MainComponent::openTrackView()
{
	// Frueher EditorComponent::OpenTrackView. Erzeugt navigator bei
	// Bedarf, haengt ihn in den trackViewport, und fuegt einen "Navigator"-
	// Sub-Tab in den editorAreaTab ein.
	if (navigator == nullptr)
	{
		trackViewport = std::make_unique<juce::Viewport>();
		navigator = std::make_shared<TrackNavigator>(
			AudioManager::getInstance()->getDeviceManager(),
			trackViewport.get(),
			propertyView,
			transportDisplay.get());

		PrefabFactory::getInstance()->init(navigator.get());
		editor->setNavigator(navigator.get());
		navigator->setEditor(editor);
		// Rechtsklick-Kontextmenues im Arrange-Bereich routen ihre Auswahl
		// hierueber zurueck in die bestehende Menue-Aktions-Logik. topLevelMenuIndex
		// ist fuer die Region-/Track-Command-IDs irrelevant (menuItemSelected
		// verzweigt nur ueber die ID).
		navigator->onMenuCommand = [this](int id) { menuItemSelected(id, 0); };
		trackViewport->setSize(500, 200);
		// Zweites Argument false: viewport NICHT als Owner — navigator ist
		// shared_ptr, also wuerde der Viewport-Dtor per default `delete
		// navigator.get()` rufen waehrend der shared_ptr noch lebt -> Double-
		// Free (SIGABRT in libsystem_malloc). Frueher in EditorComponent stand
		// hier auch nur `setViewedComponent(navigator.get())` ohne den false,
		// aber dort wurde Viewport vor navigator zerstoert und der Bug blieb
		// latent.
		trackViewport->setViewedComponent(navigator.get(), false);
		trackViewport->setScrollBarsShown(true, true);
		trackViewport->setScrollOnDragEnabled(false);
		trackViewport->setInterceptsMouseClicks(true, true);

		// Frueher: trackView->addMouseListener(editorComponent, true). Der
		// EditorComponent ist weg — Listener-Pfad ueber MainComponent ist
		// in der heutigen Architektur ungenutzt (kein MainComponent::mouseUp
		// auf navigator-Klicks). Weglassen.

		if (auto* tl = getTopLevelComponent())
			tl->addKeyListener(navigator.get());
		navigator->setMouseClickGrabsKeyboardFocus(true);
		navigator->setWantsKeyboardFocus(true);
		navigator->setInterceptsMouseClicks(true, true);
		navigator->toFront(false);
	}

	if (editorAreaTab == nullptr) return;

	// Doppel-Tab vermeiden, sonst landet jedes openTrackView() in einem
	// neuen "Navigator" Tab.
	for (int i = 0; i < editorAreaTab->getNumTabs(); ++i)
	{
		if (editorAreaTab->getTabNames()[i] == "Navigator")
		{
			editorAreaTab->setCurrentTabIndex(i);
			navigator->setInterceptsMouseClicks(true, true);
			navigator->toFront(true);
			return;
		}
	}
	editorAreaTab->addTab("Navigator", juce::Colours::grey, trackViewport.get(), false);
	editorAreaTab->setCurrentTabIndex(editorAreaTab->getNumTabs() - 1);
	navigator->setInterceptsMouseClicks(true, true);
	navigator->toFront(true);
}

void MainComponent::importAudio()
{
	if (navigator == nullptr || navigator->getCurrentTrack() == nullptr) return;

	FileChooser chooser("Select a file to add...");
	if (! chooser.browseForFileToOpen()) return;
	File file = chooser.getResult();
	const auto ext = file.getFileExtension().toLowerCase();
	if (ext == ".wav" || ext == ".aif" || ext == ".mp3")
	{
		navigator->getCurrentTrack()->addRegion(
			file.getFileNameWithoutExtension(), file, this->sampleRate);
		Project::getInstance()->addAudioFile(
			file.getFileNameWithoutExtension(), file.getFullPathName());
		navigator->repaint();
	}
}

void MainComponent::importMidi()
{
	if (navigator == nullptr || navigator->getCurrentTrack() == nullptr)
	{
		juce::AlertWindow::showMessageBox(juce::AlertWindow::WarningIcon,
			"No Track Selected",
			"Please select a MIDI or Instrument track before importing MIDI files.");
		return;
	}

	Track* currentTrack = navigator->getCurrentTrack();
	if (currentTrack->getType() != Track::MIDI && currentTrack->getType() != Track::INSTRUMENT)
	{
		juce::AlertWindow::showMessageBox(juce::AlertWindow::WarningIcon,
			"Invalid Track Type",
			"MIDI files can only be imported to MIDI or Instrument tracks.");
		return;
	}

	FileChooser chooser("Select MIDI file to import...",
		File::getSpecialLocation(File::userDocumentsDirectory),
		"*.mid;*.midi");
	if (! chooser.browseForFileToOpen()) return;
	File file = chooser.getResult();
	const auto ext = file.getFileExtension().toLowerCase();
	if (ext != ".mid" && ext != ".midi")
	{
		juce::AlertWindow::showMessageBox(juce::AlertWindow::WarningIcon,
			"Invalid File Type", "Please select a valid MIDI file (.mid or .midi)");
		return;
	}

	juce::MidiFile midiFile;
	juce::FileInputStream fileStream(file);
	if (midiFile.readFrom(fileStream))
	{
		importMidiFileToTrack(midiFile, currentTrack);
		navigator->repaint();
		juce::Logger::getCurrentLogger()->writeToLog(
			"Successfully imported MIDI file: " + file.getFileName());
	}
	else
	{
		juce::AlertWindow::showMessageBox(juce::AlertWindow::WarningIcon,
			"Import Error", "Failed to read MIDI file: " + file.getFileName());
	}
}

void MainComponent::importMidiFileToTrack(const juce::MidiFile& midiFile, Track* track)
{
	if (! track || midiFile.getNumTracks() == 0) return;

	const long startPosition = navigator->getSamplePosition();
	double midiLengthInSeconds = midiFile.getLastTimestamp();
	long regionLengthInSamples = (long)(midiLengthInSeconds * this->sampleRate);
	if (regionLengthInSamples < (long) this->sampleRate)
		regionLengthInSamples = (long) this->sampleRate;

	MidiRegion* region = track->addMidiRegion(this->sampleRate, startPosition, regionLengthInSamples);
	if (! region) {
		juce::Logger::getCurrentLogger()->writeToLog("Failed to create MIDI region");
		return;
	}

	for (int trackIndex = 0; trackIndex < midiFile.getNumTracks(); ++trackIndex)
	{
		const juce::MidiMessageSequence* sequence = midiFile.getTrack(trackIndex);
		if (! sequence) continue;

		double ticksPerSecond = midiFile.getTimeFormat();
		if (midiFile.getTimeFormat() < 0)
		{
			const int framesPerSecond = -(midiFile.getTimeFormat() >> 8);
			const int ticksPerFrame   = midiFile.getTimeFormat() & 0xFF;
			ticksPerSecond = framesPerSecond * ticksPerFrame;
		}
		else
		{
			const double bpm = 120.0;
			ticksPerSecond = (bpm / 60.0) * midiFile.getTimeFormat();
		}

		for (int i = 0; i < sequence->getNumEvents(); ++i)
		{
			auto* event = sequence->getEventPointer(i);
			if (! event) continue;
			const juce::MidiMessage& message = event->message;

			const double timeInSeconds = message.getTimeStamp() / ticksPerSecond;
			const int    sampleOffset  = (int)(timeInSeconds * this->sampleRate);

			if (message.isNoteOnOrOff() || message.isController() || message.isPitchWheel())
			{
				juce::MidiMessage* newMessage = nullptr;
				if (message.isNoteOn())
					newMessage = new juce::MidiMessage(juce::MidiMessage::noteOn(
						track->getMidiChannel(), message.getNoteNumber(), message.getVelocity()));
				else if (message.isNoteOff())
					newMessage = new juce::MidiMessage(juce::MidiMessage::noteOff(
						track->getMidiChannel(), message.getNoteNumber(), message.getVelocity()));
				else if (message.isController())
					newMessage = new juce::MidiMessage(juce::MidiMessage::controllerEvent(
						track->getMidiChannel(), message.getControllerNumber(), message.getControllerValue()));
				else if (message.isPitchWheel())
					newMessage = new juce::MidiMessage(juce::MidiMessage::pitchWheel(
						track->getMidiChannel(), message.getPitchWheelValue()));
				else
					newMessage = new juce::MidiMessage(message);

				newMessage->setTimeStamp(sampleOffset);
				region->addMessage(newMessage, timeInSeconds, sampleOffset);
			}
		}
	}

	region->rebuildThumbnail();
	region->updateThumb();
	region->repaint();
	juce::Logger::getCurrentLogger()->writeToLog(
		"Imported MIDI file with " + juce::String(midiFile.getNumTracks()) + " tracks");
}

void MainComponent::showOrCreateLogPanel()
{
	if (dockManager == nullptr) return;
	if (auto* p = dockManager->findPanel("log"))
	{
		dockManager->activatePanel(p);
		return;
	}
	auto* panel = dockManager->createPanel(
		"log", "Log",
		std::make_unique<LogView>(),
		docking::DockSide::Center);
	if (panel != nullptr) panel->setClosable(true);
	logDockPanel = panel;
}

void MainComponent::showOrCreateLevelMeterPanel()
{
	if (dockManager == nullptr) return;
	if (auto* p = dockManager->findPanel("level"))
	{
		dockManager->activatePanel(p);
		return;
	}
	auto holder = std::make_unique<AudioRefHolder<AudioLevelMeter>>(
		std::make_unique<AudioLevelMeter>(), levelMeterAudio);
	auto* panel = dockManager->createPanel(
		"level", "Level Meter",
		std::move(holder),
		docking::DockSide::Center);
	if (panel != nullptr) panel->setClosable(true);
	levelDockPanel = panel;
}

void MainComponent::showOrCreateSpectrumPanel()
{
	if (dockManager == nullptr) return;
	if (auto* p = dockManager->findPanel("spectrum"))
	{
		dockManager->activatePanel(p);
		return;
	}
	auto fresh = std::make_unique<SpectrumAnalyzer>();
	fresh->setNumberOfBars(128);
	auto holder = std::make_unique<AudioRefHolder<SpectrumAnalyzer>>(
		std::move(fresh), spectrumAudio);
	auto* panel = dockManager->createPanel(
		"spectrum", "Spektrum",
		std::move(holder),
		docking::DockSide::Center);
	if (panel != nullptr) panel->setClosable(true);
	spectrumDockPanel = panel;
}

void MainComponent::showOrCreatePianoRollPanel(std::unique_ptr<PianoRollEditor> editor)
{
	if (dockManager == nullptr || editor == nullptr) return;

	// Bestehendes Piano-Roll-Panel weg — der Aufrufer hat den Editor schon
	// frisch fuer die jetzt geklickte MidiRegion verkabelt (Callbacks
	// binden auf MidiRegion*). Wir versuchen NICHT, den Content im alten
	// Panel umzubiegen, sonst leakt der alte PianoRollEditor und seine
	// Callbacks zeigen weiter auf die alte Region.
	if (auto* existing = dockManager->findPanel("pianoroll"))
		dockManager->closePanel(existing);

	auto* panel = dockManager->createPanel(
		"pianoroll", "Piano Roll",
		std::unique_ptr<juce::Component>(editor.release()),
		docking::DockSide::Center);
	if (panel != nullptr)
	{
		panel->setClosable(true);

		// Standardmaessig landet ein Center-Panel im ersten Tab-Node (Editor,
		// oben/links). Stattdessen in den unteren Tab-Bereich zum Mixer
		// tabifizieren, sofern der Mixer (noch) gedockt ist.
		if (auto* mixer = dockManager->findPanel("mixer"))
			if (auto* mixerTabNode = dynamic_cast<docking::DockTabNode*>(
					mixer->getParentComponent()))
				dockManager->dockPanel(panel, mixerTabNode, docking::DockSide::Center);

		dockManager->activatePanel(panel);
	}
}

void MainComponent::closePianoRollPanel()
{
	if (dockManager == nullptr) return;
	if (auto* p = dockManager->findPanel("pianoroll"))
		dockManager->closePanel(p);
}

void MainComponent::showOrCreateEventEditorPanel(std::unique_ptr<MidiRegionEventEditor> editor)
{
	if (dockManager == nullptr || editor == nullptr) return;

	// Selbe Logik wie beim PianoRoll: ein evtl. bestehendes Panel wird
	// verworfen, weil der Editor auf eine konkrete MidiRegion verkabelt
	// wurde — Content im alten Panel zu ersetzen wuerde den alten Editor
	// leaken und Callbacks an die alte Region zeigen lassen.
	if (auto* existing = dockManager->findPanel("eventeditor"))
		dockManager->closePanel(existing);

	auto* panel = dockManager->createPanel(
		"eventeditor", "Event Editor",
		std::unique_ptr<juce::Component>(editor.release()),
		docking::DockSide::Center);
	if (panel != nullptr)
	{
		panel->setClosable(true);

		// Wie beim PianoRoll: in den unteren Tab-Bereich zum Mixer tabifizieren
		// statt im Editor-Tab oben/links zu landen.
		if (auto* mixer = dockManager->findPanel("mixer"))
			if (auto* mixerTabNode = dynamic_cast<docking::DockTabNode*>(
					mixer->getParentComponent()))
				dockManager->dockPanel(panel, mixerTabNode, docking::DockSide::Center);

		dockManager->activatePanel(panel);
	}
}

void MainComponent::closeEventEditorPanel()
{
	if (dockManager == nullptr) return;
	if (auto* p = dockManager->findPanel("eventeditor"))
		dockManager->closePanel(p);
}

juce::File MainComponent::getDockLayoutFile() const
{
	auto dir = juce::File::getSpecialLocation(juce::File::userHomeDirectory)
				   .getChildFile(".Synthlab");
	if (! dir.exists())
		dir.createDirectory();
	return dir.getChildFile("layout.xml");
}

void MainComponent::saveDockLayout()
{
	if (dockManager == nullptr) return;
	// Maximiertes Node liegt ausserhalb des Layout-Baums — vor dem Speichern
	// zurueckhaengen, sonst fehlt es in der persistierten Struktur.
	dockManager->restoreMaximizedNode();
	const auto state = dockManager->saveLayout();
	auto xml = state.createXml();
	if (xml != nullptr)
		xml->writeTo(getDockLayoutFile(), {});
}

void MainComponent::loadDockLayout()
{
	if (dockManager == nullptr) return;
	const auto file = getDockLayoutFile();
	if (! file.existsAsFile()) return;

	auto xml = juce::XmlDocument(file).getDocumentElement();
	if (xml == nullptr) return;
	const auto state = juce::ValueTree::fromXml(*xml);
	if (state.isValid())
		dockManager->loadLayout(state);
}

void MainComponent::resetDockLayout()
{
	// Layout-File loeschen + Reset-Aufruf entfaellt — die einfachste
	// "Defaults"-Strategie ist ein App-Neustart, weil die initialen
	// createPanel-Aufrufe in createStudioLayout die Default-Anordnung
	// herstellen. Wir loeschen nur das File, damit der naechste Start sauber
	// ist; der aktuelle DockManager bleibt mit dem aktuellen Layout im
	// Speicher (keine Notwendigkeit, den User mitten in der Session
	// zurueckzusetzen).
	const auto file = getDockLayoutFile();
	if (file.existsAsFile())
		file.deleteFile();
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

		// Studio-Layout: DockManager belegt den ganzen Mittelbereich. Toolbar
		// und Statusbar liegen ausserhalb, Reservierung passiert oben durch
		// `r`. Der frueher hier sitzende StretchableLayoutManager mit drei
		// festen Pane-Items ist weg — Anordnung, Resize und Floating uebernimmt
		// jetzt der Manager.
		if (dockManager != nullptr) {
			dockManager->setBounds(r.getX(), r.getY() + 25, r.getWidth(), r.getHeight());
		}

		if (getParentComponent() != nullptr) {
			if (cpuLoadLabel != nullptr) {
				cpuLoadLabel->setBounds(getParentComponent()->getWidth() - 50, 0, 50, 20);
				loadSlider->setBounds(getParentComponent()->getWidth() - 150, 10, 100, 10);
			}
			if (editorViewport != nullptr) {
				editor->resized();
			}
			if (menu != nullptr) {
				menu->setBounds(0, 0, getParentWidth(), 25);
			}

		}
	}
	else {
		// Player-Modus (kein DockManager) — editor liegt im Wesentlichen pur
		// im MainComponent. Frueher hier: editorView->setSize(parent size)
		// (das EditorComponent als Voll-Container). Mit der Aufloesung wird
		// der editor direkt skaliert; resized() rechnet sein internes Canvas.
		if (getParentComponent() != nullptr && editor != nullptr) {
			editor->setSize(getParentWidth(), getParentHeight());
			editor->resized();
		}
	}
}

namespace {
    // Base ID for dynamically-generated Macros menu entries. Existing IDs in
    // this file: 5..15, 41..49, 60..80, 200..207, undoCommand, 999, 1000+i
    // (samples), 2000+i (recent). 5000+ gives us plenty of headroom for nested
    // macro folders without collisions.
    constexpr int kMacroMenuIdBase   = 5000;
    constexpr int kTestMenuIdBase    = 7000;
    constexpr int kRunAllTestsId     = 6999;  // sits just before the test range
    // Sitzt oberhalb der Test-Range. Indizes [0..n) korrespondieren zu
    // MainComponent::moduleTrackFiles und werden bei Klick im Edit-Menue
    // ausgewertet.
    constexpr int kModuleTrackMenuIdBase = 9000;

    juce::File getMacroDir()
    {
        auto dir = juce::File::getSpecialLocation (juce::File::userHomeDirectory)
                       .getChildFile (".Synthlab/macros");
        if (! dir.exists())
            dir.createDirectory();  // best-effort; if it fails the menu just stays empty
        return dir;
    }

    juce::File getTestDir()
    {
        // User asked for ".Synthlab/test" (singular) — respecting that spelling.
        auto dir = juce::File::getSpecialLocation (juce::File::userHomeDirectory)
                       .getChildFile (".Synthlab/test");
        if (! dir.exists())
            dir.createDirectory();
        return dir;
    }

    struct ByFileName
    {
        int compareElements (const juce::File& a, const juce::File& b) const
        {
            return a.getFileName().compareNatural (b.getFileName());
        }
    };

    // Recursively populate `menu` from `dir`: subdirectories become submenus
    // (skipped if they contain no .js anywhere underneath, so the user does
    // not see empty branches), .js files become items. `files` is the flat
    // lookup table used by menuItemSelected — its index plus `idBase` is the
    // assigned menu ID. Shared by both the Macros menu and the Tests menu;
    // the only difference is the ID base they pass in.
    void buildScriptMenu (const juce::File& dir,
                          juce::PopupMenu& menu,
                          std::vector<juce::File>& files,
                          int idBase)
    {
        juce::Array<juce::File> subdirs;
        dir.findChildFiles (subdirs, juce::File::findDirectories, false);
        ByFileName cmp;
        subdirs.sort (cmp);

        for (const auto& sub : subdirs)
        {
            juce::PopupMenu sm;
            const size_t beforeCount = files.size();
            buildScriptMenu (sub, sm, files, idBase);
            if (files.size() != beforeCount)  // only show non-empty branches
                menu.addSubMenu (sub.getFileName(), sm);
        }

        juce::Array<juce::File> jsFiles;
        dir.findChildFiles (jsFiles, juce::File::findFiles, false, "*.js");
        jsFiles.sort (cmp);

        for (const auto& f : jsFiles)
        {
            const int id = idBase + (int) files.size();
            files.push_back (f);
            menu.addItem (id, f.getFileNameWithoutExtension());
        }
    }

    juce::File getUserModulesDirForTrackPicker()
    {
        auto dir = juce::File::getSpecialLocation(juce::File::userHomeDirectory)
                       .getChildFile(".Synthlab/modules");
        if (!dir.exists())
            dir.createDirectory();
        return dir;
    }

    // Liefert true, wenn die .slb-Datei isInstrument=1 setzt. Wir parsen das
    // XML hier on-demand pro Menue-Aufbau — das ist deutlich seltener als ein
    // Audio-Block und im Vergleich zum Plugin-Scanner trivial. ValueTree-
    // Property-Lookup statt eines Streams, damit das auch fuer Legacy-Dateien
    // ohne das Attribut sauber 0 zurueckliefert.
    bool slbIsMarkedInstrument(const juce::File& file)
    {
        std::unique_ptr<juce::XmlElement> xml = juce::XmlDocument(file).getDocumentElement();
        if (xml == nullptr) return false;
        juce::ValueTree v = juce::ValueTree::fromXml(*xml);
        return v.getProperty("isInstrument").toString().getIntValue() > 0;
    }

    // Identisch in Form zu buildScriptMenu, aber filtert .slb-Files nach
    // isInstrument=1. Subordner-Aeste werden uebersprungen, wenn sie nichts
    // Passendes enthalten.
    void buildModuleTrackMenu(const juce::File& dir,
                              juce::PopupMenu& menu,
                              std::vector<juce::File>& files)
    {
        juce::Array<juce::File> subdirs;
        dir.findChildFiles(subdirs, juce::File::findDirectories, false);
        ByFileName cmp;
        subdirs.sort(cmp);

        for (const auto& sub : subdirs)
        {
            juce::PopupMenu sm;
            const size_t beforeCount = files.size();
            buildModuleTrackMenu(sub, sm, files);
            if (files.size() != beforeCount)
                menu.addSubMenu(sub.getFileName(), sm);
        }

        juce::Array<juce::File> slbFiles;
        dir.findChildFiles(slbFiles, juce::File::findFiles, false, "*.slb");
        slbFiles.sort(cmp);

        for (const auto& f : slbFiles)
        {
            if (!slbIsMarkedInstrument(f))
                continue;
            const int id = kModuleTrackMenuIdBase + (int) files.size();
            files.push_back(f);
            menu.addItem(id, f.getFileNameWithoutExtension());
        }
    }
}

bool MainComponent::buildAddModuleTrackMenu(juce::PopupMenu& menu)
{
    moduleTrackFiles.clear();
    buildModuleTrackMenu(getUserModulesDirForTrackPicker(), menu, moduleTrackFiles);
    return !moduleTrackFiles.empty();
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

		menu.addSeparator();
		menu.addItem(17, "License Server...", true, false, nullptr);
		menu.addItem(16, "Deactivate License", true, false, nullptr);
		menu.addItem(999, "Exit", true, false, nullptr);
	}
	else if (index == 1) {
#if defined(JUCE_PLUGINHOST_AU) || defined(JUCE_PLUGINHOST_VST)
		menu.addItem(200, "Manage plugins", true, false, nullptr);
#endif
		menu.addSeparator();
		menu.addItem(201, "Tracks", true, false, nullptr);
		menu.addItem(202, "Console", true, false, nullptr);
		menu.addItem(205, "Quickstart", true, false, nullptr);
		menu.addSeparator();
		// DockablePanel-Show-Items. isTicked = panel-ist-aktuell-offen.
		// "Show Properties / Editor / JavaScript" sind nicht aufgefuehrt —
		// diese drei sind nicht-schliessbar und damit immer da.
		auto isPanelOpen = [this](const juce::String& id) {
			return dockManager != nullptr && dockManager->findPanel(id) != nullptr;
		};
		menu.addItem(210, "Show Mixer",      true, isPanelOpen("mixer"));
		menu.addItem(211, "Show Tools",      true, isPanelOpen("tools"));
		menu.addItem(206, "Show Log",        true, isPanelOpen("log"));
		menu.addItem(203, "Show Level Meter",true, isPanelOpen("level"));
		menu.addItem(204, "Show Spektrum",   true, isPanelOpen("spectrum"));
		menu.addItem(207, "JavaScript Editor", true, false, nullptr);
		menu.addSeparator();
		menu.addItem(212, "Reset Dock Layout", true, false, nullptr);
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
		// "Add Module Track" expandiert zu allen User-Modulen (.slb in
		// ~/.Synthlab/modules/) mit isInstrument=1. Bleibt disabled, wenn keines
		// existiert — sonst klickt der User ins Leere. Die Picker-Item-IDs
		// liegen in einem hohen Bereich (ab kModuleTrackMenuIdBase, s.u.) und
		// werden im Handler ausgewertet.
		{
			juce::PopupMenu moduleTracks;
			const bool hasAny = buildAddModuleTrackMenu(moduleTracks);
			menu.addSubMenu("Add Module Track", moduleTracks, hasAny);
		}
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
		menu.addItem(90, "Auto Arrange...", true, false, nullptr);
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

	// Macros: dispatched by name rather than index because the index differs
	// between plugin-host and non-plugin-host builds (see getMenuBarNames).
	if (menuName == "Macros") {
		macroFiles.clear();
		const auto dir = getMacroDir();
		buildScriptMenu (dir, menu, macroFiles, kMacroMenuIdBase);
	}

	// Tests: dedicated menu rooted at ~/.Synthlab/test/. Has a fixed "Run All
	// Tests" entry at the top, then a separator, then the discovered files.
	if (menuName == "Tests") {
		testFiles.clear();
		const auto dir = getTestDir();
		// "Run All" added first; only enabled if there's actually something to run.
		// We tentatively add it now and let menuItemSelected handle the empty case.
		menu.addItem (kRunAllTestsId, "Run All Tests");
		menu.addSeparator();

		buildScriptMenu (dir, menu, testFiles, kTestMenuIdBase);
	}

	return menu;
}

void MainComponent::menuItemSelected(int menuItemID, int topLevelMenuIndex) {

	// Run All Tests — fires before the test-ID range so it has its own
	// sentinel ID (sits just below kTestMenuIdBase).
	if (menuItemID == kRunAllTestsId) {
		if (jsEditor == nullptr || testFiles.empty()) return;

		int passed = 0, failed = 0;
		jsEditor->appendOutput (
			juce::String ("==== Running ") + juce::String ((int) testFiles.size())
				+ " test file(s) ====\n",
			juce::Colours::cornflowerblue);

		for (const auto& f : testFiles) {
			if (jsEditor->runFile (f)) ++passed;
			else                       ++failed;
		}

		const juce::String summary =
			juce::String ("==== Test summary: ") + juce::String (passed) + " passed, "
				+ juce::String (failed) + " failed ====\n";
		jsEditor->appendOutput (summary,
			failed == 0 ? juce::Colours::lightgreen : juce::Colours::tomato);
		return;
	}

	// Add-Module-Track Picker: IDs ab kModuleTrackMenuIdBase indizieren
	// moduleTrackFiles. Vor der Test-Range pruefen, weil hoeher liegend.
	if (menuItemID >= kModuleTrackMenuIdBase) {
		const size_t idx = (size_t) (menuItemID - kModuleTrackMenuIdBase);
		if (idx < moduleTrackFiles.size()) {
			if (navigator.get() == nullptr) {
				openTrackView();
				navigator.get()->addChangeListener(this);
			}
			auto* action = new AddTrackAction(editor, navigator.get(),
			                                  moduleTrackFiles[idx]);
			Project::getInstance()->getUndoManager()->beginNewTransaction();
			Project::getInstance()->getUndoManager()->perform(action);
			editor->repaint();
		}
		return;
	}

	// Tests: individual file IDs sit in [kTestMenuIdBase, kMacroMenuIdBase).
	// Check this BEFORE the macro range because kMacroMenuIdBase < kTestMenuIdBase
	// would otherwise grab everything (the macros check is "id >= 5000").
	if (menuItemID >= kTestMenuIdBase) {
		const size_t idx = (size_t) (menuItemID - kTestMenuIdBase);
		if (idx < testFiles.size() && jsEditor != nullptr)
			jsEditor->runFile (testFiles[idx]);
		return;
	}

	// Macros: IDs starting at kMacroMenuIdBase index into macroFiles, which
	// was populated by the most recent getMenuForIndex("Macros", ...) call.
	if (menuItemID >= kMacroMenuIdBase) {
		const size_t idx = (size_t) (menuItemID - kMacroMenuIdBase);
		if (idx < macroFiles.size() && jsEditor != nullptr)
			jsEditor->runFile (macroFiles[idx]);
		return;
	}

	if (menuItemID == 5) {
		openSettings();
	}
	else if (menuItemID == 6) {
		importAudio();
	}
	else if (menuItemID == 12) {
		importMidi();
	}
	else if (menuItemID == 13) {
		showBounceDialog();
	}
	else if (menuItemID == 14) {
		showClaudeApiKeyDialog();
	}
	else if (menuItemID == 16) {
		showDeactivateLicenseDialog();
	}
	else if (menuItemID == 17) {
		showLicenseServerDialog();
	}
	else if (menuItemID == 15) {
		FileChooser chooser("Select a folder containing stems...");
		if (chooser.browseForDirectory()) {
			File folder = chooser.getResult();

			if (navigator.get() == nullptr) {
				openTrackView();
				navigator.get()->addChangeListener(this);
			}

			Array<File> wavFiles;
			folder.findChildFiles(wavFiles, File::findFiles, false, "*.wav");
			wavFiles.sort();

			const double sr = Project::getInstance()->getSampleRate();

			for (const File& file : wavFiles) {
				Track* t = TrackUtil::createAudioTrack(editor, navigator.get(), editor->getModule());
				if (t != nullptr) {
					t->setName(file.getFileNameWithoutExtension());
					t->addRegion(file.getFileNameWithoutExtension(), file, sr);
					Project::getInstance()->addAudioFile(file.getFileNameWithoutExtension(), file.getFullPathName());
				}
			}

			navigator.get()->adjustHeight();
			navigator.get()->repaint();
			editor->repaint();
		}
	}
	else if (menuItemID == 7) {
		navigator.get()->setPlaying(true);
	}
	else if (menuItemID == 7) {
		navigator.get()->setPlaying(false);
	}

#if defined(JUCE_PLUGINHOST_AU) || defined(JUCE_PLUGINHOST_VST)
	else if (menuItemID == 200) {
		PluginManager::getInstance()->scanPlugins();
	}
#endif
	else if (menuItemID == 201) {
		openTrackView();
		if (navigator.get() != nullptr)
			navigator.get()->addChangeListener(this);
	}
	else if (menuItemID == 202) {
		// "Console" Item — fuegt einen ShellConsoleComponent-Tab in das
		// Tools-Panel (toolsTab) ein. War frueher in EditorComponent::openConsole;
		// MainComponent uebernimmt das jetzt, weil der Tools-Tab hier verwaltet
		// wird.
		if (toolsTab != nullptr && parser != nullptr) {
			auto* console = new ShellConsoleComponent(*parser);
			toolsTab->addTab("Console", Colours::darkgrey, console, true);
			toolsTab->setCurrentTabIndex(toolsTab->getNumTabs() - 1);
			if (toolsDockPanel != nullptr) dockManager->activatePanel(toolsDockPanel);
		}
	}

	else if (menuItemID == 203) {
		showOrCreateLevelMeterPanel();
	}
	else if (menuItemID == 204) {
		showOrCreateSpectrumPanel();
	}
	else if (menuItemID == 205) {
		openQuickstart();
	}
	else if (menuItemID == 206) {
		showOrCreateLogPanel();
	}
	else if (menuItemID == 207) {
		openJavaScriptEditor();
	}
	else if (menuItemID == 210) {
		// Show Mixer — Mixer ist nicht-schliessbar, also nur Activate. Falls
		// jemand die Closability spaeter umstellt, wuerde hier ein Re-Create
		// noetig.
		if (dockManager != nullptr)
			if (auto* p = dockManager->findPanel("mixer"))
				dockManager->activatePanel(p);
	}
	else if (menuItemID == 211) {
		// Show Tools — analog, Tools ist nicht-schliessbar.
		if (dockManager != nullptr)
			if (auto* p = dockManager->findPanel("tools"))
				dockManager->activatePanel(p);
	}
	else if (menuItemID == 212) {
		// Reset Dock Layout — Layout-File loeschen, dem User Hinweis, dass
		// der Restart das Default-Layout wiederherstellt. (Mid-Session reset
		// wuerde das aktuelle DockManager-Tree umbauen — komplex und nicht
		// gefragt.)
		resetDockLayout();
		juce::AlertWindow::showMessageBox(juce::AlertWindow::InfoIcon,
			"Reset Dock Layout",
			"Das gespeicherte Dock-Layout wurde geloescht. Beim naechsten Start "
			"erscheinen die Panels wieder in der Default-Anordnung.");
	}

	else if (menuItemID == 8) {

		if (navigator.get() == nullptr) {
			openTrackView();
			navigator.get()->addChangeListener(this);
		}
		auto* action = new AddTrackAction(editor, navigator.get(),
		                                  AddTrackAction::Type::Audio);
		Project::getInstance()->getUndoManager()->beginNewTransaction();
		Project::getInstance()->getUndoManager()->perform(action);
		editor->repaint();
	}
	else if (menuItemID == 9) {
		if (navigator.get() == nullptr) {
			openTrackView();
			navigator.get()->addChangeListener(this);
		}
		auto* action = new AddTrackAction(editor, navigator.get(),
		                                  AddTrackAction::Type::Midi);
		Project::getInstance()->getUndoManager()->beginNewTransaction();
		Project::getInstance()->getUndoManager()->perform(action);
		editor->repaint();
	}
	else if (menuItemID == 10) {
		if (navigator.get() == nullptr) {
			openTrackView();
			navigator.get()->addChangeListener(this);
		}
		editor->addVirtualInstrumentTrack();
		editor->repaint();
	}
	else if (menuItemID == 11)
	{
		if (navigator.get()->getCurrentTrack() != nullptr)
		{
			const double sr  = Project::getInstance()->getSampleRate();
			const float  bpm = Project::getInstance()->getTempo();
			const int    beatsPerBar = 4;
			const long   oneBarSamples = (long)(beatsPerBar * (60.0 / bpm) * sr);

			navigator.get()->getCurrentTrack()->addMidiRegion(
				sr,
				navigator.get()->getSamplePosition(),
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
		// Duplicate ueber ALLE Tracks dispatchen — selektierte Regionen koennen
		// auf beliebig vielen Tracks liegen, der Track-Header muss dafuer nicht
		// "currentTrack" sein.
		for (auto* t : navigator->getTracks())
			if (t != nullptr) t->duplicateSelectedRegions();
	}
	else if (menuItemID == 44) {
		// Split-at-Marker laeuft auf allen Tracks am aktuellen Playhead —
		// Selektion irrelevant, kein currentTrack-Gate noetig.
		for (auto* t : navigator->getTracks())
			if (t != nullptr) t->splitRegion();
	}
	else if (menuItemID == 45) {
		for (int i = 0; i < navigator->getTracks().size(); i++) {
			navigator->getTracks().at(i)->removeSelectedRegions(true);
		}
	}
	else if (menuItemID == 46) {
		// Reverse auf jede selektierte AudioRegion ueber alle Tracks. Kein
		// currentTrack-Gate — das uebersieht man im UI zu leicht und die Aktion
		// schluckt sich dann lautlos.
		for (auto* t : navigator->getTracks())
			if (t != nullptr) t->reverseSelectedAudioRegions();
	}
	else if (menuItemID == 47) {
		NormalizeRegionAction* action = new NormalizeRegionAction(navigator.get());
		Project::getInstance()->getUndoManager()->beginNewTransaction();
		Project::getInstance()->getUndoManager()->perform(action);

	}
	else if (menuItemID == 48) {
		FadeRegionAction* action = new FadeRegionAction(navigator.get());
		Project::getInstance()->getUndoManager()->beginNewTransaction();
		Project::getInstance()->getUndoManager()->perform(action);
	}
	else if (menuItemID == 49) {
		// Pro Track: mehrere selektierte Regionen → mergen (Ergebnis ist per
		// Definition schon konsolidiert, sss=0, visible == buffer). Sonst alle
		// einzeln selektierten Regionen auf dem Track konsolidieren. Kein
		// currentTrack-Gate — Aktion greift fuer jede selektierte Region,
		// unabhaengig davon ob der Track-Header gerade fokussiert ist.
		for (auto* t : navigator->getTracks()) {
			if (t == nullptr) continue;
			if (t->mergeSelectedRegions()) continue;
			t->consolidateSelectedRegions();
		}
	}
	else if (menuItemID == 60) {
		if (navigator != nullptr && navigator->getCurrentTrack() != nullptr) {
			auto* action = new TrackDeleteAction(navigator.get(), editor);
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
	else if (menuItemID == 90) { // Auto Arrange...
		showAutoArrangeDialog();
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
	const char* const names[] = { "File", "View","Plugins", "Edit", "Region", "Macros", "Tests", nullptr };
#else
	const char* const names[] = { "File", "View", "Edit", "Macros", "Tests", nullptr };
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
			if (self->editorViewport != nullptr
				&& self->navigator.get() != nullptr
				&& !self->navigator.get()->getTracks().empty())
			{
				self->openTrackView();
			}
		}
	});
}

void MainComponent::openQuickstart() {
	if (editorViewport == nullptr)
		return;

	auto* tabs = editorAreaTab;
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
			if (navigator.get() == nullptr) {
				openTrackView();
				navigator.get()->addChangeListener(this);
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
			auto* nav = navigator.get();
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

JavaScriptEditorComponent* MainComponent::createJavaScriptEditorComponent() {
	auto* c = new JavaScriptEditorComponent();
	jsEditor = c;
	jsEditor->setTransportNavigator(navigator.get());
	jsEditor->setSynthEditor(editor);
	// openTrackView ist jetzt MainComponent-Methode; Callback hier reichen.
	jsEditor->setOpenTrackViewCallback([this]() { openTrackView(); });
	jsEditor->setTransportDisplay(transportDisplay.get());
	// Publish to Project so modules (e.g. TriggerScriptModule) can reach the
	// JS editor without each one needing its own setter wiring from here.
	Project::getInstance()->setJavaScriptEditor(jsEditor);
	return c;
}

void MainComponent::showOrCreateJavaScriptPanel() {
	if (dockManager == nullptr) return;

	if (auto* existing = dockManager->findPanel("javascript")) {
		dockManager->activatePanel(existing);
	}
	else {
		auto* jsEditorOwned = createJavaScriptEditorComponent();
		jsDockPanel = dockManager->createPanel(
			"javascript", "JavaScript",
			std::unique_ptr<juce::Component>(jsEditorOwned),
			docking::DockSide::Right);
		if (jsDockPanel != nullptr) {
			jsDockPanel->setClosable(true);
			jsDockPanel->onClosed = [this]() {
				jsEditor    = nullptr;
				jsDockPanel = nullptr;
				if (auto* proj = Project::getInstance()) proj->setJavaScriptEditor(nullptr);
			};
			dockManager->activatePanel(jsDockPanel);
		}
	}

	if (jsEditor != nullptr)
		jsEditor->grabKeyboardFocus();
}

void MainComponent::openJavaScriptEditor() {
	// Menue-Eintrag: JS-Panel in den Vordergrund holen — oder neu anlegen,
	// falls der User es geschlossen hat.
	showOrCreateJavaScriptPanel();
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

void MainComponent::showDeactivateLicenseDialog()
{
	const bool ok = juce::AlertWindow::showOkCancelBox(
		juce::AlertWindow::QuestionIcon,
		"Deactivate License",
		"This device will be unregistered from the license server, freeing up the "
		"seat for another installation. Synthlab will need to be activated again "
		"afterwards. Continue?",
		"Deactivate", "Cancel", this, nullptr);

	if (! ok)
		return;

	// deactivate() makes a short network call. Runs synchronously here since the
	// user deliberately triggered the action and waits for the result.
	const bool success = LicenseManager::getInstance()->deactivate();

	if (success)
	{
		juce::AlertWindow::showMessageBox(
			juce::AlertWindow::InfoIcon, "Deactivate License",
			"This device has been deactivated. Please restart Synthlab to activate "
			"it again.");
	}
	else
	{
		juce::AlertWindow::showMessageBox(
			juce::AlertWindow::WarningIcon, "Deactivate License",
			"Deactivation failed (could not reach the license server?). "
			"Nothing was changed.");
	}
}

void MainComponent::showLicenseServerDialog()
{
	auto* w = new juce::AlertWindow(
		"License Server",
		"Base URL of the license server (without the trailing /activate). "
		"Leave empty to use the built-in default.",
		juce::AlertWindow::NoIcon);

	w->setLookAndFeel(Project::getInstance()->getLookAndFeel());
	w->addTextEditor("url", Settings::getInstance()->getLicenseServerUrl(), "URL:");
	w->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey));
	w->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

	w->enterModalState(true,
		juce::ModalCallbackFunction::create([w](int result)
		{
			if (result == 1)
				Settings::getInstance()->setLicenseServerUrl(w->getTextEditorContents("url"));
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

SynthEditor* MainComponent::getActiveSubEditor() const {
	// Studio-Layout: topTab[0] = Viewport(root editor), topTab[1..] = Viewport(sub editor).
	// Player-Layout: kein editorView, dann gibt es nur den Root.
	if (editorViewport == nullptr) return editor;
	auto* tabs = editorAreaTab;
	if (tabs == nullptr) return editor;
	if (auto* vp = dynamic_cast<juce::Viewport*>(tabs->getCurrentContentComponent())) {
		if (auto* se = dynamic_cast<SynthEditor*>(vp->getViewedComponent()))
			return se;
	}
	return editor;
}

bool MainComponent::keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) {
	// Globaler Start/Stop-Shortcut. TrackNavigator::keyPressed greift nur,
	// wenn der Navigator den Fokus hat — sobald irgendein anderes Panel
	// (Property-Editor-Canvas, Mixer, leerer Tab usw.) den Fokus hat, kommt
	// der Key hier oben am MainComponent an. TextEditor-Felder konsumieren
	// Space selbst (Eingabe), also wird das Toggle nicht beim Tippen
	// ausgeloest.
	if (key == juce::KeyPress::spaceKey) {
		if (editorViewport != nullptr) {
			if (auto* nav = navigator.get())
				nav->togglePlayStop();
		}
		return true;
	}

	// Modifier-Shortcuts (Cmd+T / +D / +C / +V / +X / +B / +L / +E / +M)
	// liegen am Navigator als zweiter KeyListener auf dem Top-Level. Wenn
	// wir hier am MainComponent-Listener unbedingt true zurueckgeben,
	// stoppt JUCE die Dispatch-Kette und der Navigator sieht sie nie. Reine
	// modifier-lose Tasten dagegen sind explizit hier oben verkabelt (y/x
	// = Oktave, l = Layer, d+Alt = Duplikat im Editor, Backspace = Delete).
	if (key.getModifiers().isCommandDown() || key.getModifiers().isCtrlDown())
		return false;

	if (key.getTextCharacter() == 'y') {
		if (currentOctave > 0) {
			currentOctave--;
		}
		return true;
	}
	if (key.getTextCharacter() == 'x') {
		if (currentOctave < 8) {
			currentOctave++;
		}
		return true;
	}
	if (key.getTextCharacter() == 'l') {
		layer = (layer + 1) % 4;
		editor->setCurrentLayer(layer);
		editor->repaint();
		return true;
	}
	if (key.getTextCharacter() == 'd' && isOptionPressed) {
		editor->duplicateSelected();
		editor->repaint();
		return true;
	}
	if (key.getKeyCode() == KeyPress::deleteKey || key.getKeyCode() == KeyPress::backspaceKey) {
		SynthEditor* active = getActiveSubEditor();
		TrackNavigator* nav = (active == editor && editorViewport != nullptr)
			? navigator.get()
			: nullptr;
		SynthEditor::deleteSelectionWithCascade(active, nav);
		return true;
	}

	// Default: nicht konsumieren — andere Listener (Navigator, Editor,
	// Command-Manager) sollen ihre Chance bekommen.
	return false;
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
			SynthEditor* active = getActiveSubEditor();
			TrackNavigator* nav = (active == editor && editorViewport != nullptr)
				? navigator.get()
				: nullptr;
			SynthEditor::deleteSelectionWithCascade(active, nav);
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
		if (navigator != nullptr && navigator->getCurrentTrack() != nullptr) {
			midiProcessor->setActiveChannel(navigator->getCurrentTrack()->getMidiChannel());
			// Identity-based live routing: the armed/selected track receives
			// live input regardless of its MIDI channel (allows >16 instruments).
			midiProcessor->setActiveTrack(navigator->getCurrentTrack());
		}
		else if (navigator != nullptr) {
			midiProcessor->setActiveTrack(nullptr);
		}
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

	// Neues Projekt geladen -> Shadow-Tree neu aufbauen. Andernfalls zeigt das
	// Panel noch Bindings auf jetzt zerstoerte Source-Module aus dem alten
	// Projekt (referTo wuerde ins Leere laufen).
	if (controlSurfacePanel != nullptr)
		controlSurfacePanel->rebuild();
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

void MainComponent::showAutoArrangeDialog()
{
	if (navigator == nullptr) return;
	TimeLine* tl = navigator->getTimeLine();
	if (tl == nullptr) return;

	const double sr           = Project::getInstance()->getSampleRate();
	const double loopStartSec = tl->getLoopStart();
	const double loopEndSec   = tl->getLoopEnd();

	if (!tl->isLoopEnabled() || (loopEndSec - loopStartSec) <= 1e-6) {
		juce::AlertWindow::showMessageBoxAsync(
			juce::AlertWindow::InfoIcon,
			"Auto Arrange",
			"Bitte zuerst einen Loop-Bereich auf der Timeline setzen — daraus wird die Songstruktur gebaut.");
		return;
	}

	const long loopStartSamples = (long) std::llround(loopStartSec * sr);
	const long blockLenSamples  = (long) std::llround((loopEndSec - loopStartSec) * sr);
	if (blockLenSamples <= 0) return;

	const double bpm           = Project::getInstance()->getTempo();
	const double samplesPerBar = (bpm > 0.0) ? (sr / (bpm / 60.0)) * 4.0 : 0.0;
	const double blockBars     = samplesPerBar > 0.0 ? (double) blockLenSamples / samplesPerBar : 0.0;
	const double blockSeconds  = (double) blockLenSamples / sr;

	auto* dialog = new AutoArrangeDialog(blockSeconds, blockBars);

	dialog->onArrange = [this, dialog, loopStartSamples, blockLenSamples, sr]() {
		// Werte VOR dem Schliessen einsammeln — exitModalState gibt den Dialog frei.
		auto       sections = dialog->getSections();
		const bool markers  = dialog->getPlaceMarkers();

		if (auto* dw = dialog->findParentComponentOfClass<juce::DialogWindow>())
			dw->exitModalState(0);

		if (sections.empty() || navigator == nullptr) return;

		auto* action = new AutoArrangeAction(navigator.get(), loopStartSamples,
		                                     blockLenSamples, std::move(sections),
		                                     markers, sr);
		Project::getInstance()->getUndoManager()->beginNewTransaction();
		Project::getInstance()->getUndoManager()->perform(action);
	};

	dialog->onCancel = [dialog]() {
		if (auto* dw = dialog->findParentComponentOfClass<juce::DialogWindow>())
			dw->exitModalState(0);
	};

	juce::DialogWindow::LaunchOptions options;
	options.content.setOwned(dialog);
	options.dialogTitle            = "Auto Arrange";
	options.dialogBackgroundColour = juce::Colour(0xff222222);
	options.escapeKeyTriggersCloseButton = true;
	options.useNativeTitleBar      = true;
	options.resizable              = false;
	options.componentToCentreAround = this;

	auto* window = options.launchAsync();
	Project::getInstance()->getOpenWindows().push_back(window);
}
