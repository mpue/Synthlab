/*
  ==============================================================================

    QuantizerModule.cpp

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "QuantizerModule.h"
#include <vector>
#include <cmath>

namespace {
    // Pitch-Class-Sets relativ zum Root. 0=Root, 1=Root+1 Halbton, etc.
    // Reihenfolge muss zur Doku in getDescription() passen.
    const std::vector<std::vector<int>> kScales = {
        { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 }, // 0: Chromatisch
        { 0, 2, 4, 5, 7, 9, 11 },                  // 1: Dur (Ionisch)
        { 0, 2, 3, 5, 7, 8, 10 },                  // 2: Moll natuerlich (Aeolisch)
        { 0, 2, 3, 5, 7, 8, 11 },                  // 3: Moll harmonisch
        { 0, 2, 4, 7, 9 },                          // 4: Pentatonisch Dur
        { 0, 3, 5, 7, 10 },                         // 5: Pentatonisch Moll
        { 0, 3, 5, 6, 7, 10 },                      // 6: Blues
        { 0, 2, 3, 5, 7, 9, 10 },                   // 7: Dorisch
        { 0, 2, 4, 5, 7, 9, 10 },                   // 8: Mixolydisch
        { 0, 2, 4, 6, 8, 10 }                       // 9: Ganzton
    };

    // Snap eine Pitch-Class (0..11) auf die naechste Stufe der Skala.
    // Gibt die gewaehlte Skalenstufe zurueck (auch 0..11) sowie ggf. ein
    // Octave-Carry (-1, 0 oder +1) falls auf eine Stufe gerundet wurde, die
    // logisch zur naechsthoeheren oder -tieferen Oktave gehoert.
    int snapPitchClass(int pc, const std::vector<int>& scale, int& octaveCarry) {
        octaveCarry = 0;
        if (scale.empty()) return pc;

        // Beste Stufe in [scale[i] ... scale[i] + (mit Octave-Wrap)]: wir
        // kandidieren jede Skalenstufe in ihrer Original-Oktave und plus 12,
        // damit wir die Stufe finden, die am naechsten an pc liegt.
        int bestPc = scale[0];
        int bestDist = std::abs(pc - bestPc);
        int bestCarry = 0;

        for (int s : scale) {
            // gleiche Oktave
            int d1 = std::abs(pc - s);
            if (d1 < bestDist) {
                bestDist = d1;
                bestPc = s;
                bestCarry = 0;
            }
            // naechste Oktave hoch (wenn s+12 naeher an pc ist als irgendwas)
            int d2 = std::abs(pc - (s + 12));
            if (d2 < bestDist) {
                bestDist = d2;
                bestPc = s;
                bestCarry = 1;
            }
            // naechste Oktave runter
            int d3 = std::abs(pc - (s - 12));
            if (d3 < bestDist) {
                bestDist = d3;
                bestPc = s;
                bestCarry = -1;
            }
        }

        octaveCarry = bestCarry;
        return bestPc;
    }
}

QuantizerModule::QuantizerModule()
{
    setSize(120, 140);
    setName("Quantizer");

    editable = false;
    prefab = true;
}

QuantizerModule::~QuantizerModule()
{
}

void QuantizerModule::configurePins() {
    // pins[0] = In (VALUE) — Pitch in Halbtoeneinheiten (typisch MIDI 0..127)
    // pins[1] = S  (VALUE) — Skalen-Index (0..N-1)
    // pins[2] = R  (VALUE) — Root-Note 0..11 (0=C)
    // pins[3] = Out (VALUE) — gerasteter Pitch
    Pin* in = new Pin(Pin::Type::VALUE);
    in->direction = Pin::Direction::IN;
    in->listeners.push_back(this);
    in->setName("In");
    in->setDescription("Eingangs-Pitch in Halbtoeneinheiten (MIDI-Range typisch).");

    Pin* scale = new Pin(Pin::Type::VALUE);
    scale->direction = Pin::Direction::IN;
    scale->listeners.push_back(this);
    scale->setName("S");
    scale->setDescription("Skalen-Index. 0=Chromatisch (Pass-Through), 1=Dur, 2=Moll nat., ... 9=Ganzton.");

    Pin* root = new Pin(Pin::Type::VALUE);
    root->direction = Pin::Direction::IN;
    root->listeners.push_back(this);
    root->setName("R");
    root->setDescription("Root-Note 0..11. Default 0 (= C).");

    Pin* out = new Pin(Pin::Type::VALUE);
    out->direction = Pin::Direction::OUT;
    out->listeners.push_back(this);
    out->setName("Out");
    out->setDescription("Gerasteter Pitch.");

    addPin(Pin::Direction::IN, in);
    addPin(Pin::Direction::IN, scale);
    addPin(Pin::Direction::IN, root);
    addPin(Pin::Direction::OUT, out);
}

void QuantizerModule::paint(juce::Graphics& g) {
    Module::paint(g);
}

void QuantizerModule::process() {
    if (pins.size() < 4) return;

    float inVal = 0.0f;
    if (pins.at(0)->getConnections().size() == 1)
        inVal = pins.at(0)->getConnections().at(0)->getValue();

    int scaleIdx = 0;
    if (pins.at(1)->getConnections().size() == 1)
        scaleIdx = static_cast<int>(std::round(pins.at(1)->getConnections().at(0)->getValue()));
    if (scaleIdx < 0) scaleIdx = 0;
    if (scaleIdx >= static_cast<int>(kScales.size()))
        scaleIdx = static_cast<int>(kScales.size()) - 1;

    int rootNote = 0;
    if (pins.at(2)->getConnections().size() == 1)
        rootNote = static_cast<int>(std::round(pins.at(2)->getConnections().at(0)->getValue()));
    rootNote = ((rootNote % 12) + 12) % 12;  // safe modulo

    // Auf Halbton runden (Quantize ist eh nur fuer ganze Halbtoenen sinnvoll).
    int note = static_cast<int>(std::round(inVal));
    // Auf Skala bezogen: vom Root abziehen, sodass 0 = Root.
    int relative = note - rootNote;
    int octave = static_cast<int>(std::floor(relative / 12.0f));
    int pc = relative - octave * 12;  // 0..11

    int octaveCarry = 0;
    int snappedPc = snapPitchClass(pc, kScales[scaleIdx], octaveCarry);
    int outNote = (octave + octaveCarry) * 12 + snappedPc + rootNote;

    pins.at(3)->setValue(static_cast<float>(outNote));
}
