#pragma once

#include <JuceHeader.h>
#include "Tracks/MidiRegion.h"
#include <stack>
#include <memory>

class PianoRollComponent : public juce::Component,
    public juce::KeyListener,
    public juce::Timer,
    public juce::ApplicationCommandTarget
{
public:
    // === CORE ===
    PianoRollComponent(MidiRegion* region, double sampleRate);
    ~PianoRollComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    // === SETTINGS ===
    void setGridResolution(int denominator);
    void setBPM(double newBPM) { bpm = newBPM; repaint(); }
    void setSnapToGrid(bool snap) { snapToGrid = snap; }
    void setQuantizeStrength(float strength) { quantizeStrength = juce::jlimit(0.0f, 1.0f, strength); }
    void setVelocitySensitivity(bool sensitive) { velocitySensitive = sensitive; }

    // === TOOLS ===
    enum class Tool {
        SELECT,     // Auswählen/Bewegen
        PENCIL,     // Noten zeichnen  
        ERASER,     // Noten löschen
        RESIZE,     // Noten dehnen
        VELOCITY,   // Velocity editieren
        SPLIT,      // Noten teilen
        GLUE        // Noten verbinden
    };
    void setCurrentTool(Tool tool) { currentTool = tool; /*setCursor(getCursorForTool(tool));*/ }
    Tool getCurrentTool() const { return currentTool; }

    // === PLAYBACK ===
    void setPlaybackPosition(double samplePosition);
    void setFollowPlayback(bool follow) { followPlayback = follow; }
    void setLoopRegion(double startSample, double endSample);
    void clearLoopRegion() { hasLoopRegion = false; repaint(); }

    // === SELECTION ===
    void selectAll();
    void selectNone();
    void selectInRange(double startTime, double endTime, int minPitch = 0, int maxPitch = 127);
    void invertSelection();
    int getSelectedNoteCount() const { return static_cast<int>(selectedMessages.size()); }

    // === CLIPBOARD ===
    void copySelected();
    void cutSelected();
    void pasteAtPosition(double timePosition, int pitch = -1);
    bool canPaste() const { return !clipboard.empty(); }

    // === QUANTIZATION ===
    void quantizeSelected(int gridDivision = 16);
    void humanizeSelected(float amount = 0.1f);
    void grooveQuantize(const std::vector<float>& grooveTemplate);

    // === VELOCITY ===
    void setSelectedVelocity(int velocity);
    void scaleSelectedVelocity(float factor);
    void randomizeSelectedVelocity(int range = 20);
    void crescendoSelected(int startVel, int endVel);

    // === TRANSFORMATION ===
    void transposeSelected(int semitones);
    void stretchSelectedTempo(float factor);
    void reverseSelected();
    void duplicateSelected(int times = 1, double interval = 0.0);

    // === CHORD & SCALE TOOLS ===
    void quantizeToScale(const std::vector<int>& scale);
    void addChord(int rootNote, double startTime, double duration, const std::vector<int>& intervals);
    void arpeggiate(double noteSpacing = 0.1);

    // === AUTOMATION ===
    struct ControllerLane {
        int ccNumber;
        juce::String name;
        std::vector<std::pair<double, int>> points; // time, value
        bool visible = false;
        juce::Colour colour;
    };
    void addControllerLane(int ccNumber, const juce::String& name, juce::Colour colour = juce::Colours::blue);
    void setControllerLaneVisibility(int ccNumber, bool visible);

    // === MOUSE & KEYBOARD ===
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    void mouseMagnify(const juce::MouseEvent& e, float scaleFactor) override;

    bool keyPressed(const juce::KeyPress& key, juce::Component*) override;
    void modifierKeysChanged(const juce::ModifierKeys& modifiers) override;

    // === COMMAND TARGET ===
    juce::ApplicationCommandTarget* getNextCommandTarget() override { return nullptr; }
    void getAllCommands(juce::Array<juce::CommandID>& commands) override;
    void getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo& result) override;
    bool perform(const InvocationInfo& info)  override;

    // === EVENTS ===
    std::function<void(const std::set<MidiMessage*>&)> onSelectionChanged;
    std::function<void(MidiMessage*, int oldVelocity, int newVelocity)> onVelocityChanged;
    std::function<void()> onRegionModified;

private:
    // === CORE DATA ===
    MidiRegion* midiRegion;
    double sampleRate;
    Tool currentTool = Tool::SELECT;

    // === GRID & TIMING ===
    float gridResolution = 1.0f / 16.0f;
    double bpm = 120.0;
    bool snapToGrid = true;
    float quantizeStrength = 1.0f;
    std::vector<int> customScale;

    // === LAYOUT ===
    int pianoWidth = 120;
    int controllerLaneHeight = 60;
    float xZoom = 0.02f;
    float yZoom = 14.0f;
    juce::Point<float> panOffset{ 0, 0 };
    int totalHeight = 128 * 14; // for piano keys

    // === PLAYBACK ===
    double playbackPosition = -1.0;
    bool followPlayback = false;
    bool hasLoopRegion = false;
    double loopStart = 0.0, loopEnd = 0.0;

    // === SELECTION & EDITING ===
    std::set<MidiMessage*> selectedMessages;
    juce::Rectangle<int> selectionRectangle;
    bool drawSelectionRect = false;

    // === MOUSE STATE ===
    juce::Point<int> dragStart;
    juce::Point<int> lastMousePos;
    bool isDragging = false;
    bool isPanning = false;
    bool isResizing = false;
    bool isBoxSelecting = false;
    bool isVelocityEditing = false;

    juce::Point<int> panStart;
    MidiMessage* resizingMessage = nullptr;
    MidiMessage* resizingPartner = nullptr;
    double originalDuration = 0.0;
    juce::Rectangle<int> originalNoteRect;

    // === VELOCITY EDITING ===
    bool velocitySensitive = true;
    std::map<MidiMessage*, int> originalVelocities;
    int baseVelocity = 100;

    // === CLIPBOARD ===
    struct ClipboardNote {
        int pitch;
        double startTime;
        double duration;
        int velocity;
        int channel;
    };
    std::vector<ClipboardNote> clipboard;
    double clipboardTimeOrigin = 0.0;

    // === UNDO/REDO ===
    struct UndoableAction {
        juce::String description;
        std::function<void()> undoFunc;
        std::function<void()> redoFunc;
    };
    std::stack<std::unique_ptr<UndoableAction>> undoStack;
    std::stack<std::unique_ptr<UndoableAction>> redoStack;
    void pushUndoAction(const juce::String& description,
        std::function<void()> undoFunc,
        std::function<void()> redoFunc);

    // === CONTROLLER AUTOMATION ===
    std::map<int, ControllerLane> controllerLanes;
    int selectedControllerLane = -1;
    bool editingController = false;

    // === VISUAL FEEDBACK ===
    juce::Colour noteColours[16]; // Per-channel colours
    bool showVelocityAsHeight = false;
    bool showVelocityAsColour = true;
    bool showNoteNames = false;
    bool showGrid = true;
    float ghostNoteAlpha = 0.3f;

    // Animation
    float pulseAnimation = 0.0f;
    juce::Colour playheadColour = juce::Colours::red;

    // === DRAWING METHODS ===
    void drawGrid(juce::Graphics& g);
    void drawPiano(juce::Graphics& g);
    void drawNotes(juce::Graphics& g);
    void drawNote(juce::Graphics& g, const juce::MidiMessage& msg, double duration, bool isSelected = false);
    void drawVelocityBars(juce::Graphics& g);
    void drawControllerLanes(juce::Graphics& g);
    void drawPlayhead(juce::Graphics& g);
    void drawLoopRegion(juce::Graphics& g);
    void drawSelectionRectangle(juce::Graphics& g);
    void drawSnapGuides(juce::Graphics& g);

    // === HELPER METHODS ===
    bool isBlackKey(int n);
    int getYForPitch(int pitch);
    int getPitchAtY(int y);
    int getXForTime(double sampleTime);
    double getTimeAtX(int x);
    double snapTimeToGrid(double time);
    int snapPitchToScale(int pitch);

    juce::Rectangle<int> getNoteRect(const juce::MidiMessage& msg, double duration);
    juce::Rectangle<int> getVelocityRect(const juce::MidiMessage& msg, double duration);

    juce::Colour getColourForVelocity(int velocity);
    juce::Colour getColourForChannel(int channel);
    juce::MouseCursor getCursorForTool(Tool tool);

    // === NOTE MANIPULATION ===
    MidiMessage* findNoteOffForNoteOn(MidiMessage* noteOn);
    double getNoteDuration(MidiMessage* noteOn);
    void addNotePair(int pitch, double startTime, double duration, int velocity = 100, int channel = 1);
    void deleteNotePair(MidiMessage* noteOn);
    void moveNote(MidiMessage* noteOn, double deltaTime, int deltaPitch);
    void resizeNote(MidiMessage* noteOn, double newDuration);
    void setNoteVelocity(MidiMessage* noteOn, int velocity);
    void splitNote(MidiMessage* noteOn, double splitTime);

    // === REGION OPERATIONS ===
    void duplicateRegion(double startTime, double endTime, double targetTime);
    void deleteRegion(double startTime, double endTime);
    void insertSilence(double atTime, double duration);

    // === HIT TESTING ===
    MidiMessage* getNoteAt(juce::Point<int> position);
    std::vector<MidiMessage*> getNotesInRect(juce::Rectangle<int> rect);
    bool isOverNoteEdge(juce::Point<int> position, MidiMessage* note, double duration);
    bool isOverVelocityHandle(juce::Point<int> position, MidiMessage* note);

    // === TOOL-SPECIFIC HANDLERS ===
    void handleSelectTool(const juce::MouseEvent& e);
    void handlePencilTool(const juce::MouseEvent& e);
    void handleEraserTool(const juce::MouseEvent& e);
    void handleVelocityTool(const juce::MouseEvent& e);
    void handleSplitTool(const juce::MouseEvent& e);

    // === NOTIFICATION ===
    void notifySelectionChanged();
    void notifyRegionModified();

    // === CONSTRAINTS ===
    double getMinTime() const { return 0.0; }
    double getMaxTime() const { return midiRegion->getNumSamples(); }

    // === COMMAND IDS ===
    enum CommandIDs {
        selectAllNotes = 0x2000,
        selectNo,
        copy,
        cut,
        paste,
        deleteSelected,
        undo,
        redo,
        quantize,
        humanize,
        transposeUp,
        transposeDown,
        velocityIncrease,
        velocityDecrease,
        duplicateNotes,
        reverseNotes,
        splitAtPlayhead
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoRollComponent)
};