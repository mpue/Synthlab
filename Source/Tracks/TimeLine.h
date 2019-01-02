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
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TimeLine)
	virtual void changeListenerCallback(ChangeBroadcaster * source) override;
	double length;
	int offset = 0;
    ScopedPointer<DropShadow> dropShadow;
    ScopedPointer<DropShadower> dropShadower;

};


#endif  // TIMELINE_H_INCLUDED
