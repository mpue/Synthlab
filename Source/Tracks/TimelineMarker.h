/*
  ==============================================================================
    TimelineMarker.h
    Wavelab-style timeline marker. "Normal" markers sind reine visuelle Cues;
    "Loop" markers triggern beim Playhead-Crossing einen Sprung zurueck zum
    vorhergehenden Marker (egal welchen Typs) und werden loopRepeats mal
    wiederholt, bevor die Wiedergabe normal weiterlaeuft.
  ==============================================================================
*/
#pragma once
#include "../JuceLibraryCode/JuceHeader.h"

struct TimelineMarker
{
    enum class Type { Normal, Loop };

    double      position    = 0.0;            // Sekunden auf der Timeline
    juce::String name;
    Type        type        = Type::Normal;
    int         loopRepeats = 1;              // # zusaetzlicher Durchlaeufe (Loop-Typ)

    // Runtime: # noch ausstehender Rueckspruenge. Wird von TrackNavigator beim
    // Play-Start aus loopRepeats befuellt und in der Audio-Thread-Schleife
    // dekrementiert. NICHT persistieren — rein transient.
    int         repeatsLeft = 0;
};
