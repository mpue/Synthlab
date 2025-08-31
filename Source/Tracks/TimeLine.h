/*
  ==============================================================================
    TimeLine.h
    Created: 25 Nov 2016 10:54:33am
    Author:  pueskma
    Extended: Loop functionality between two locators
  ==============================================================================
*/
#ifndef TIMELINE_H_INCLUDED
#define TIMELINE_H_INCLUDED
#include "../JuceLibraryCode/JuceHeader.h"
#include "TimeLine.h"
//==============================================================================
/*
*/
class TimeLine : public Component, public ChangeListener
{
public:
    TimeLine(double lengthInSeconds);
    ~TimeLine();
    void paint(Graphics&) override;
    void resized() override;
    void mouseDown(const MouseEvent& event) override;
    void mouseDrag(const MouseEvent& event) override;
    void mouseUp(const MouseEvent& event) override;

    void setLength(double length);
    static String secondsToTime(int seconds);
    void setOffset(int offset);
    void setTempoAndTimeSignature(double bpm, int numerator, int denominator);

    // Loop-Funktionalität
    void setLoopEnabled(bool enabled);
    bool isLoopEnabled() const { return loopEnabled; }
    void setLoopStart(double startTime);
    void setLoopEnd(double endTime);
    double getLoopStart() const { return loopStart; }
    double getLoopEnd() const { return loopEnd; }
    void setLoopRegion(double startTime, double endTime);
    void clearLoop();

    // Callback für Loop-Änderungen
    std::function<void(double, double, bool)> onLoopChanged;

private:
    virtual void changeListenerCallback(ChangeBroadcaster* source) override;
    double pixelToTime(float pixel) const;
    float timeToPixel(double time) const;
    bool isMouseOverLocator(const MouseEvent& event, double locatorTime, float tolerance = 5.0f) const;

    double length;
    int offset = 0;
    std::unique_ptr<DropShadow> dropShadow;
    std::unique_ptr<DropShadower> dropShadower;
    double bpm = 120.0;
    int timeSigNumerator = 4;
    int timeSigDenominator = 4;

    // Loop-Parameter
    bool loopEnabled = false;
    double loopStart = 0.0;
    double loopEnd = 0.0;

    // Mouse-Interaktion
    enum DragMode {
        None,
        DragLoopStart,
        DragLoopEnd,
        DragLoopRegion
    };

    DragMode currentDragMode = None;
    double dragStartOffset = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimeLine)
};
#endif  // TIMELINE_H_INCLUDED