/*
  ==============================================================================

    WaveSelector.h
    Created: 25 Nov 2016 8:48:48am
    Author:  pueskma

  ==============================================================================
*/

#ifndef WAVESELECTOR_H_INCLUDED
#define WAVESELECTOR_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"

//==============================================================================
/*
*/
class WaveSelector    : public Component, public ChangeBroadcaster
{
public:
    WaveSelector();
    ~WaveSelector();

    void paint (Graphics&) override;
    void resized() override;

	void setSelectedRange(int start, int end);
    
    Rectangle<int> getSelectedRange();

private:
	int selectionStart;
	int selectionEnd;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveSelector)
};


#endif  // WAVESELECTOR_H_INCLUDED
