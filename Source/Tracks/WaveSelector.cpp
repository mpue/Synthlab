/*
  ==============================================================================

    WaveSelector.cpp
    Created: 25 Nov 2016 8:48:48am
    Author:  pueskma

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "WaveSelector.h"
#include <cmath>

//==============================================================================
WaveSelector::WaveSelector()
{
	this->selectionStart = 0;
	this->selectionEnd = 0;
    setInterceptsMouseClicks(false, false);
}

WaveSelector::~WaveSelector()
{
}

void WaveSelector::paint (Graphics& g) {
	
	g.setColour(Colour::fromFloatRGBA(0.0f, 0.5f, 0.5f, 0.5f));

    if (selectionEnd > 0) {
        g.fillRect(selectionStart, 0, selectionEnd, this->getHeight());
    }
    else {
        g.fillRect(selectionStart + selectionEnd, 0, abs(selectionEnd), this->getHeight());
    }
	
}

void WaveSelector::resized() {

}

Rectangle<int> WaveSelector::getSelectedRange() {
    
    Rectangle<int> range;
    range.setX(selectionStart);
    range.setWidth(selectionEnd);
    return range;
}

void WaveSelector::setSelectedRange(int start, int end){
    Logger::getCurrentLogger()->writeToLog("Selection range : "+String(start)+","+String(end));
    
    this->selectionStart = start;
	this->selectionEnd = end;
	repaint();
    sendChangeMessage();
}
