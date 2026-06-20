/*
  ==============================================================================

    StepSequencer8Module.cpp

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "StepSequencer8Module.h"

StepSequencer8Module::StepSequencer8Module(double sampleRate, int buffersize)
{
    this->sampleRate = sampleRate;
    this->buffersize = buffersize;

    // 11 Input-Pins (Clk, Rst, Gate, S1..S8) liegen auf 20px-Raster ab y=10,
    // der letzte bei y=210 — plus Platz fuer das Namens-Label unten.
    setSize(150, 270);
    setName("8-Step Seq");

    editable = false;
    prefab = true;
}

StepSequencer8Module::~StepSequencer8Module()
{
}

void StepSequencer8Module::configurePins() {
    // Reihenfolge bestimmt die vertikale Anordnung. Quelle wird ueber die
    // Pin-Pointer (clockIn/resetIn/gateIn) disambiguiert, nicht ueber Index —
    // Pin::sendEvent stempelt den liefernden Pin als Source ans Event.
    clockIn = new Pin(Pin::Type::EVENT);
    clockIn->direction = Pin::Direction::IN;
    clockIn->listeners.push_back(this);
    clockIn->setName("Clk");
    clockIn->setDescription("Clock-Event. Jede steigende Flanke rueckt den aktiven Step um 1 vor (wrappt nach Step 8).");
    addPin(Pin::Direction::IN, clockIn);

    resetIn = new Pin(Pin::Type::EVENT);
    resetIn->direction = Pin::Direction::IN;
    resetIn->listeners.push_back(this);
    resetIn->setName("Rst");
    resetIn->setDescription("Reset-Event. Springt auf Step 1 (Index 0) zurueck.");
    addPin(Pin::Direction::IN, resetIn);

    gateIn = new Pin(Pin::Type::EVENT);
    gateIn->direction = Pin::Direction::IN;
    gateIn->listeners.push_back(this);
    gateIn->setName("Gate");
    gateIn->setDescription("Zweiter Advance-Trigger, ODER-verknuepft mit Clk. Steigende Flanke schaltet weiter.");
    addPin(Pin::Direction::IN, gateIn);

    for (int i = 0; i < kNumSteps; ++i) {
        Pin* s = new Pin(Pin::Type::VALUE);
        s->direction = Pin::Direction::IN;
        s->listeners.push_back(this);
        s->setName("S" + juce::String(i + 1));
        s->setDescription("Wert fuer Step " + juce::String(i + 1) + ". Unverbunden = 0.");
        addPin(Pin::Direction::IN, s);
        stepIns[i] = s;
    }

    valueOut = new Pin(Pin::Type::VALUE);
    valueOut->direction = Pin::Direction::OUT;
    valueOut->listeners.push_back(this);
    valueOut->setName("Value");
    valueOut->setDescription("Wert des aktuell aktiven Steps (live vom jeweiligen S-Pin).");
    addPin(Pin::Direction::OUT, valueOut);

    stepOut = new Pin(Pin::Type::VALUE);
    stepOut->direction = Pin::Direction::OUT;
    stepOut->listeners.push_back(this);
    stepOut->setName("Step");
    stepOut->setDescription("Aktueller Step-Index 0..7 als VALUE-Out (z.B. fuer LED/Display).");
    addPin(Pin::Direction::OUT, stepOut);

    gateOut = new Pin(Pin::Type::EVENT);
    gateOut->direction = Pin::Direction::OUT;
    gateOut->listeners.push_back(this);
    gateOut->setName("Gate");
    gateOut->setDescription("Gate-Out. Feuert bei jedem Step ein Gate-On (und schliesst beim Weiterschalten das vorige) zum Triggern von Envelopes.");
    addPin(Pin::Direction::OUT, gateOut);

    publishOutputs();
}

void StepSequencer8Module::sendGate(int velocity) {
    if (gateOut == nullptr) return;
    Event* g = new Event("seq gate", Event::Type::GATE);
    g->setValue(velocity);
    g->setNote(kGateNote);
    for (auto* c : gateOut->getConnections())
        c->sendEvent(new Event(g));
    delete g;
}

float StepSequencer8Module::readStepValue(int step) const {
    if (step < 0 || step >= kNumSteps) return 0.0f;
    Pin* s = stepIns[step];
    if (s != nullptr && s->getConnections().size() == 1)
        return s->getConnections().at(0)->getValue();
    return 0.0f;
}

void StepSequencer8Module::publishOutputs() {
    const int step = currentStep.load();
    if (stepOut  != nullptr) stepOut->setValue((float) step);
    if (valueOut != nullptr) valueOut->setValue(readStepValue(step));
}

void StepSequencer8Module::paint(juce::Graphics& g) {
    Module::paint(g);

    // Kleine Step-Anzeige in der Mitte: 8 Kaestchen, das aktive hervorgehoben.
    const int step = currentStep.load();
    const int boxW = 12, boxH = 12, gap = 2;
    const int totalW = kNumSteps * boxW + (kNumSteps - 1) * gap;
    const int startX = (getWidth() - totalW) / 2;
    const int y = getHeight() / 2 - boxH / 2;

    for (int i = 0; i < kNumSteps; ++i) {
        const int x = startX + i * (boxW + gap);
        if (i == step) {
            g.setColour(juce::Colours::orange);
            g.fillRect(x, y, boxW, boxH);
        } else {
            g.setColour(juce::Colours::darkgrey);
            g.fillRect(x, y, boxW, boxH);
        }
        g.setColour(juce::Colours::black);
        g.drawRect(x, y, boxW, boxH, 1);
    }

    g.setColour(juce::Colours::white);
    g.setFont(11.0f);
    g.drawText("Step " + juce::String(step + 1) + " = " + juce::String(readStepValue(step), 2),
               0, y + boxH + 4, getWidth(), 14, juce::Justification::centred);
}

void StepSequencer8Module::process() {
    // Value-Out laeuft live mit: dreht der User waehrend eines Steps am
    // zugehoerigen S-Knob, folgt der Ausgang sofort. Step-Out aendert sich
    // nur bei Clock/Reset und wird dort in publishOutputs() gesetzt.
    if (valueOut != nullptr)
        valueOut->setValue(readStepValue(currentStep.load()));
}

void StepSequencer8Module::eventReceived(Event* e) {
    if (e == nullptr) return;

    const auto type = e->getType();

    // Transport-Start: taktbuendig auf Step 1 (Index 0) scharf stellen. Der
    // erste Clock-Tick nach Play spielt dann Step 1, ohne vorher zu advancen
    // (siehe armed unten). Kein Gate hier — das feuert erst der erste Tick.
    if (type == Event::Type::CLOCK_START) {
        currentStep.store(0);
        armed.store(true);
        publishOutputs();        // Value zeigt sofort S1
        return;
    }

    // Nur echte Trigger (Clock-Event oder Gate-On) reagieren; Gate-Off / Note-Off
    // wuerden sonst doppelt schalten.
    const bool isTrigger =
        type == Event::Type::CLOCK
        || (type == Event::Type::GATE && e->getValue() > 0);
    if (!isTrigger) return;

    Pin* src = e->getSource();

    // eventReceived laeuft (je nach Quelle) auf dem Audio- ODER Message-Thread.
    // Hier NICHT repaint() aufrufen — das wuerde im Audio-Fall GUI-Arbeit auf
    // den Audio-Thread ziehen und unter Last (Tooltip/Fenster malen) Dropouts
    // verursachen. Die Anzeige liest den Live-Zustand und wird vom 33ms-Editor-
    // Timer aufgefrischt.
    if (src == resetIn && resetIn != nullptr) {
        currentStep.store(0);
        armed.store(true);       // wie ein Start: der naechste Tick spielt Step 1
        publishOutputs();
        sendGate(0);             // Envelope loslassen, Gate schliesst beim Reset
        return;
    }

    // Clk oder Gate (oder src==nullptr-Fallback fuer Events, die nicht ueber
    // Pin::sendEvent laufen) ruecken einen Step vor.
    if (src == clockIn || src == gateIn || src == nullptr) {
        // Direkt nach Start/Reset NICHT advancen: der erste Tick spielt Step 1.
        if (!armed.exchange(false)) {
            const int next = (currentStep.load() + 1) % kNumSteps;
            currentStep.store(next);
        }
        publishOutputs();
        // Gate pro Step retriggern: erst das vorige schliessen, dann fuer den
        // neuen Step oeffnen — so durchlaeuft der Envelope bei jedem Step seine
        // Attack-Phase neu und haelt Sustain bis zum naechsten Step.
        sendGate(0);
        sendGate(kGateVelocity);
    }
}
