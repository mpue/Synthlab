/*
  ==============================================================================

    TimeLine.h
    Created: 25 Nov 2016 10:54:33am
    Author:  pueskma

  ==============================================================================
*/

#ifndef TIMELINE_H_INCLUDED
#define TIMELINE_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"
#include "TimeLine.h"

//==============================================================================
/*
*/
class TimeLine    : public Component, public ChangeListener
{
public:
    TimeLine(double lengthInSeconds);
    ~TimeLine();

    void paint (Graphics&) override;
    void resized() override;
    void setLength(double length);
	static String secondsToTime(int seconds);
	void setOffset(int offset);
    void setTempoAndTimeSignature(double bpm, int numerator, int denominator);

private:
	virtual void changeListenerCallback(ChangeBroadcaster * source) override;
	double length;
	int offset = 0;
    std::unique_ptr<DropShadow> dropShadow;
    std::unique_ptr<DropShadower> dropShadower;
    double bpm = 120.0;
    int timeSigNumerator = 4;
    int timeSigDenominator = 4;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TimeLine)
};


#endif  // TIMELINE_H_INCLUDED
