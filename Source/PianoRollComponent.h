// PianoRollComponent.h
#pragma once
#include <JuceHeader.h>

class PianoRollComponent : public juce::Component,
    public juce::KeyListener,
    public juce::MouseListener
{
public:
    PianoRollComponent(juce::MidiMessageSequence& sequence);

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

    bool keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) override;

    void setGridResolution(int denominator);

private:
    void drawGrid(juce::Graphics& g);
    void drawPiano(juce::Graphics& g);
    void drawNotes(juce::Graphics& g);

    void drawNote(juce::Graphics& g, const juce::MidiMessage& msg, double duration, bool selected);
    bool isBlackKey(int noteNumber);
    int getPitchAtY(int y);
    double getTimeAtX(int x);
    int getYForPitch(int pitch);
    int getXForTime(double time);

    juce::Rectangle<int> getNoteRect(const juce::MidiMessage& msg, double duration);

    juce::MidiMessageSequence& midiSequence;
    std::set<int> selectedIndices;

    float xZoom = 100.0f;
    float yZoom = 12.0f;
    float gridResolution = 1.0f / 16.0f;
    int pianoWidth = 60;
    juce::Point<int> dragStart;
    bool isDragging = false;
    bool isResizing = false;
    int resizingIndex = -1;
    double originalDuration = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoRollComponent)
};
