//
//  Region.cpp
//  WaveX
//
//  Created by Matthias Pueski on 28.12.16.
//
//

#include "Region.h"
#include "../Actions/MoveRegionAction.h"

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

void Region::setSampleOffset(long offset, bool reminder, bool notify)
{

    if (reminder)
        this->oldOffset = this->sampleOffset;
    
    this->sampleOffset = offset;
    
    if (sampleOffset < 0) {
        sampleOffset = 0;
    }

	int bufferSize = Project::getInstance()->getBufferSize();

    this->sampleOffset = (sampleOffset / bufferSize) * bufferSize;

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

long Region::getSampleOffset() {
    return (this->sampleOffset / Project::getInstance()->getBufferSize()) * Project::getInstance()->getBufferSize();
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
    // weder Auswahl noch Drag-Vorbereitung anstossen.
    if (e.mods.isMiddleButtonDown())
        return;

    dragStartSampleOffset = getSampleOffset();
    if (dragger != nullptr)
        dragger->handleMouseDown(this, e);
}

void Region::mouseUp(const MouseEvent & e)
{
    if (e.mods.isMiddleButtonDown())
        return;

    if (dragger != nullptr)
        dragger->handleMouseUp(this, e);

    long endOffset = getSampleOffset();
    if (endOffset != dragStartSampleOffset) {
        Project::getInstance()->getUndoManager()->beginNewTransaction();
        Project::getInstance()->getUndoManager()->perform(
            new MoveRegionAction(this, dragStartSampleOffset, endOffset));
    }
}

void Region::mouseDrag(const MouseEvent & e)
{
    if (e.mods.isMiddleButtonDown())
        return;

    if (dragger != nullptr)
        dragger->handleMouseDrag(e);
}

juce::Array<juce::PropertyComponent*>& Region::getProperties()
{
    nameProp = new TextPropertyComponent(*nameValue, "Name", 16, false, true);
    properties = Array<PropertyComponent*>();
    properties.add(nameProp);
    return properties;
}

void Region::createProperties()
{
    nameValue = new Value();
    nameListener = new NameListener(*nameValue, this);
    nameValue->setValue(name);
}

void Region::setDirty(bool dirty) {
    this->dirty = dirty;
}

