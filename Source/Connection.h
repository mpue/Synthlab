/*
  ==============================================================================

    Connection.h
    Created: 2 Apr 2018 5:18:02pm
    Author:  mpue

  ==============================================================================
*/

#pragma once

// #include "Module.h"
#include "Pin.h"
#include "../JuceLibraryCode/JuceHeader.h"
class Module;
class Connection : public juce::Component
{
public:
    void paint (juce::Graphics& g) override;
    void resized() override;
	Connection();
	Connection(Module* source, Pin* output,	Module* target, Pin* input);
	~Connection();
	Module* source = nullptr;
	Pin* a;
	Module* target = nullptr;
	Pin* b;
	bool selected = false;
    void setPoints(juce::Point<int>& p1, juce::Point<int>& p2);
    juce::Path getPath();

    // Wegpunkte in Editor-Koordinaten. Die Verbindung wird als Polylinie
    // p1 -> waypoints... -> p2 ausschliesslich mit geraden Segmenten
    // gezeichnet. Doppelklick fuegt einen Punkt ein bzw. loescht ihn,
    // Maus-Drag verschiebt ihn (Logik im SynthEditor).
    std::vector<juce::Point<int>> waypoints;

    // Baseline der Wegpunkte fuer einen Move (analog Module::savePosition):
    // beim Drag-Start gesichert, in SynthEditor::moveSelection als Bezug fuer
    // das Mitschieben und in MoveSelectedAction fuer Undo verwendet.
    std::vector<juce::Point<int>> savedWaypoints;
    void saveWaypoints() { savedWaypoints = waypoints; }

    /** Index des Wegpunkts innerhalb radius um pos, sonst -1. */
    int  findWaypoint (juce::Point<int> pos, float radius) const;
    /** Fuegt pos in das naechstgelegene Segment der Polylinie ein. */
    void insertWaypoint (juce::Point<int> pos);
    void removeWaypoint (int index);

    juce::Point<int> getStartPoint() const { return p1; }
    juce::Point<int> getEndPoint()   const { return p2; }
private:
    juce::Path linePath;
    juce::Point<int> p1;
    juce::Point<int> p2;
};

