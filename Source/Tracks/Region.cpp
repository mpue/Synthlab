//
//  Region.cpp
//  WaveX
//
//  Created by Matthias Pueski on 28.12.16.
//
//

#include "Region.h"

Region::Region() {
    
}

Region::~Region() {
    
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
    
    int newOffset = (this->sampleOffset / this->sampleRate) * zoom;
    
    this->setOffset(newOffset);
    this->setTopLeftPosition(newOffset, 0);
    
    /*
     resizerR->setSize(5, getHeight());
     resizerR->setTopLeftPosition(getWidth() - 5, 0);
     
     resizerL->setSize(5, getHeight());
     resizerL->setTopLeftPosition(0, 0);
     */
    // setBounds(0,0,this->thumbnail->getTotalLength() * this->zoom, 200);

    
    dragger->setRaster(this->zoom / 4);
    
    repaint();
}

void Region::mouseDown(const MouseEvent & e)
{
    if (dragger != NULL)
        dragger->handleMouseDown(this, e);
}

void Region::mouseUp(const MouseEvent & e)
{
    if (dragger != NULL)
        dragger->handleMouseUp(this, e);
}

void Region::mouseDrag(const MouseEvent & e)
{
    if (dragger != NULL)
        dragger->handleMouseDrag(e);
}