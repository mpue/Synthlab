//
//  Region.cpp
//  WaveX
//
//  Created by Matthias Pueski on 28.12.16.
//
//

#include "Region.h"
#include "../Actions/MoveRegionAction.h"
#include "../Project/Project.h"
#include "../PropertyView.h"

Region::Region() {
    oldOffset = 0;
    createProperties();
	setAlwaysOnTop(true);
}

Region::~Region() {
    // Remove the listener before destroying it so the Value's shared data
    // does not hold a dangling pointer if any code writes to it afterwards.
    if (nameValue)
        nameValue->removeListener(nameListener);
    delete nameListener;
    delete nameValue;

    if (lengthValue)
        lengthValue->removeListener(lengthListener);
    delete lengthListener;
    delete lengthValue;

    // resizerR/resizerL are base-class members created by subclass constructors.
    // Deleting here covers all subclasses (MidiRegion, AudioRegion, etc.)
    // so subclasses must NOT delete them themselves to avoid double-free.
    delete resizerR;
    delete resizerL;
}

void Region::setName(juce::String name) {
    this->name = name;    
}

String Region::getName() {
    return name;
}

void Region::setDragger(MultiComponentDragger *dragger) {
    this->dragger = dragger;
}

void Region::setSelected(bool selected)
{
    this->selected = selected;
    this->dragger->setSelected(this, false);
    repaint();
}

bool Region::isSelected()
{
    return this->selected;
}

void Region::setLoop(bool loop)
{
    this->loop = loop;
}

bool Region::isLoop()
{
    return this->loop;
}

int Region::getLoopCount() {
    return this->loopCount;
}

void Region::setLoopCount(int count) {
    this->loopCount = count;
}

void Region::timerCallback() {
    this->updateThumb();
}

void Region::setSampleOffset(long offset, bool reminder, bool notify, bool quantize)
{

    if (reminder)
        this->oldOffset = this->sampleOffset;

    this->sampleOffset = offset;

    if (sampleOffset < 0) {
        sampleOffset = 0;
    }

    if (quantize) {
        int bufferSize = Project::getInstance()->getBufferSize();
        this->sampleOffset = (sampleOffset / bufferSize) * bufferSize;
    }

    if (!reminder)
        this->oldOffset = this->sampleOffset;

    if (notify)
        sendChangeMessage();
}
/**
 * Returns the offset before this region was moved or resized, this is needed in order to clear 
 * a range of samples of the tracks audio buffer.
 */

long Region::getOldOffset() {
    return this->oldOffset;
}

long Region::getSampleOffset() const {
    // Frueher: (sampleOffset / bufferSize) * bufferSize — Block-Alignment fuer
    // einen Audio-Pfad, der laengst per-Sample addressiert (Track::getSample).
    // Die versteckte Floor-Quantisierung hat bei jedem Read bis zu (bufferSize-1)
    // Samples verschluckt; sichtbar wurde das beim Duplikat einer konsolidierten
    // Region, deren sampleOffset (sample-genau geschrieben) hier wieder
    // abgerundet wurde — die Audio-Engine glaubte dann, das Duplikat beginne
    // (bufferSize-1) Samples frueher, getRegionAt routete die ersten Samples an
    // den Vorgaenger, das Duplikat sprang mit Offset in den Buffer und klang
    // entsprechend zu kurz. Schreiber, die mit quantize=true (default) durch
    // setSampleOffset gehen, speichern ohnehin schon bufferSize-aligned — fuer
    // die ist das hier ein No-Op.
    return this->sampleOffset;
}


void Region::setOffset(int offset)
{
    this->offset = offset;
}

double Region::getSampleRate() {
    return this->sampleRate;
}

int Region::getOffset()
{
    return offset;
}

void Region::setZoom(float zoom) {

    this->zoom = zoom;
    
    int newOffset = (int)std::round((this->sampleOffset / this->sampleRate) * zoom);
    
    this->setOffset(newOffset);
    this->setTopLeftPosition(newOffset, 0);
    this->updateThumb();
    double bpm = Project::getInstance()->getTempo();
    int raster = (int)std::round(zoom * 30.0 / bpm); // pixels per 8th note, BPM-aware
    dragger->setRaster(raster);
    
    repaint();
}

void Region::mouseDown(const MouseEvent & e)
{
    // Mittlere Maustaste ist fuer's Pannen des Track-Navigators reserviert —
    // weder Auswahl noch Drag-Vorbereitung anstossen. Rechtsklick ebenso:
    // den uebernimmt TrackNavigator (Kontextmenue + selektions-erhaltende
    // Auswahl) — wuerde der Dragger hier handleMouseDown bekommen, kollabierte
    // er eine bestehende Mehrfach-Selektion auf diese eine Region.
    if (e.mods.isMiddleButtonDown() || e.mods.isRightButtonDown())
        return;

    dragStartSampleOffset = getSampleOffset();
    if (dragger != nullptr) {
        dragger->handleMouseDown(this, e);

        // Multi-Region-Drag: Start-Offset auch fuer alle anderen selektierten
        // Regions festhalten. Sonst pushen wir spaeter im mouseUp nur fuer den
        // Leader eine MoveRegionAction, und Undo bewegt nur diese eine zurueck.
        for (auto& wc : dragger->getSelectedComponents()) {
            if (auto* c = wc.get()) {
                if (c == this) continue;
                if (auto* r = dynamic_cast<Region*>(c))
                    r->dragStartSampleOffset = r->getSampleOffset();
            }
        }
    }

    // Direkte Klicks auf eine Region updaten den PropertyView nicht ueber
    // den Dragger — bisher zeigte er nur Track-/Module-Properties an. Wir
    // pushen die Selektion hier explizit nach, damit Name, Pitch und (bei
    // AudioRegions) die Tempo-Follow Properties sichtbar werden.
    if (auto* pv = Project::getInstance()->getPropertyView())
        pv->selectionChanged(this);
}

void Region::mouseUp(const MouseEvent & e)
{
    if (e.mods.isMiddleButtonDown() || e.mods.isRightButtonDown())
        return;

    // Selektion vor handleMouseUp einsammeln — der Dragger deselektiert dort
    // ggf. den Leader (Click ohne Drag), wir wollen aber die Liste der
    // potenziell bewegten Regions bewahren.
    std::vector<Region*> draggedRegions;
    bool sawThis = false;
    if (dragger != nullptr) {
        for (auto& wc : dragger->getSelectedComponents()) {
            if (auto* c = wc.get()) {
                if (auto* r = dynamic_cast<Region*>(c)) {
                    draggedRegions.push_back(r);
                    if (r == this) sawThis = true;
                }
            }
        }
    }
    if (!sawThis)
        draggedRegions.push_back(this);

    if (dragger != nullptr)
        dragger->handleMouseUp(this, e);

    // Eine Transaktion fuer den gesamten Multi-Drag: alle MoveRegionActions
    // werden zwischen den beginNewTransaction-Aufrufen zu einem Undo-Schritt
    // zusammengefasst.
    auto* um = Project::getInstance()->getUndoManager();
    bool transactionStarted = false;
    for (Region* r : draggedRegions) {
        const long endOffset = r->getSampleOffset();
        if (endOffset != r->dragStartSampleOffset) {
            if (!transactionStarted) {
                um->beginNewTransaction();
                transactionStarted = true;
            }
            um->perform(new MoveRegionAction(r, r->dragStartSampleOffset, endOffset));
        }
    }
}

void Region::mouseDrag(const MouseEvent & e)
{
    if (e.mods.isMiddleButtonDown() || e.mods.isRightButtonDown())
        return;

    if (dragger != nullptr)
        dragger->handleMouseDrag(e);
}

juce::Array<juce::PropertyComponent*>& Region::getProperties()
{
    nameProp = new TextPropertyComponent(*nameValue, "Name", 16, false, true);
    properties = Array<PropertyComponent*>();
    properties.add(nameProp);

    // Length im Format "bars.quarters.sixteenths". Bidirektional: Anzeige
    // wird bei jedem Panel-Refresh aus dem aktuellen sample-count gefuellt,
    // Edit (Enter / Focus-Loss) parst zurueck und ruft setLengthSamples().
    const double bpm = Project::getInstance()->getTempo();
    if (lengthValue != nullptr) {
        // setValue dedupliziert ueber operator==, ein Refresh ohne Aenderung
        // feuert den LengthListener also nicht -> keine Resize-Recursion.
        lengthValue->setValue(samplesToBarQuarterSixteenth((long) getNumSamples(), bpm, sampleRate));
    }
    lengthProp = new TextPropertyComponent(*lengthValue, "Length (bar.qrt.16th)", 16, false, true);
    properties.add(lengthProp);

    return properties;
}

void Region::createProperties()
{
    nameValue = new Value();
    nameListener = new NameListener(*nameValue, this);
    nameValue->setValue(name);

    lengthValue = new Value();
    lengthListener = new LengthListener(*lengthValue, this);
    // Initialer Wert ist hier (im Region-Ctor) noch nicht sinnvoll berechen-
    // bar: getNumSamples() ist pure-virtual, der Subklassen-Ctor hat den
    // Sample-Count noch nicht gesetzt. getProperties() befuellt den Wert bei
    // jedem Panel-Refresh aus dem aktuellen Stand.
    lengthValue->setValue("0.0.0");
}

juce::String Region::samplesToBarQuarterSixteenth(long numSamples, double bpm, double sr)
{
    if (bpm <= 0.0 || sr <= 0.0 || numSamples <= 0)
        return "0.0.0";
    // 4/4 Annahme — passt zum Rest der Codebase (snapToQuarterNote etc.).
    const double sixteenthSamples = (60.0 / bpm) * sr / 4.0;
    const long totalSixteenths = (long) std::llround((double) numSamples / sixteenthSamples);
    const long bars       = totalSixteenths / 16;
    const long quarters   = (totalSixteenths % 16) / 4;
    const long sixteenths = totalSixteenths % 4;
    return juce::String(bars) + "." + juce::String(quarters) + "." + juce::String(sixteenths);
}

long Region::barQuarterSixteenthToSamples(const juce::String& text, double bpm, double sr)
{
    if (bpm <= 0.0 || sr <= 0.0)
        return -1;
    juce::StringArray parts;
    parts.addTokens(text.trim(), ".", "");
    if (parts.size() == 0)
        return -1;
    // Akzeptiert "B", "B.Q" oder "B.Q.S". Overflow ist erlaubt — "0.0.32" wird
    // korrekt zu 2 Bars, "0.5.0" zu 1 Bar + 1 Quarter. Negative Teile ergeben
    // ein negatives Ergebnis und werden vom Listener verworfen.
    const long bars       = parts[0].getIntValue();
    const long quarters   = parts.size() > 1 ? parts[1].getIntValue() : 0;
    const long sixteenths = parts.size() > 2 ? parts[2].getIntValue() : 0;
    const long totalSixteenths = bars * 16 + quarters * 4 + sixteenths;
    if (totalSixteenths <= 0)
        return -1;
    const double sixteenthSamples = (60.0 / bpm) * sr / 4.0;
    return (long) std::llround((double) totalSixteenths * sixteenthSamples);
}

void Region::setDirty(bool dirty) {
    this->dirty = dirty;
}

