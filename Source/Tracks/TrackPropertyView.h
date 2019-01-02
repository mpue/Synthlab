/*
  ==============================================================================

  This is an automatically generated GUI class created by the Projucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Projucer version: 4.3.0

  ------------------------------------------------------------------------------

  The Projucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright (c) 2015 - ROLI Ltd.

  ==============================================================================
*/

#ifndef __JUCE_HEADER_6CA99BC07CBFC596__
#define __JUCE_HEADER_6CA99BC07CBFC596__

//[Headers]     -- You can add your own extra header files here --
#include "../JuceLibraryCode/JuceHeader.h"
#include "TrackPropertyPanel.h"
#include <vector>
#include "Track.h"
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Projucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class TrackPropertyView  : public Component,
                           public ChangeListener,
                           public Timer,
                           public ComponentListener
{
public:
    //==============================================================================
    TrackPropertyView ();
    ~TrackPropertyView();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
    virtual void changeListenerCallback (ChangeBroadcaster* source) override;
    void addTrack(Track* track);
    void timerCallback() override;
	Track* getTrack(int i);
	void setOffset(int offset);
	void mouseDown(const MouseEvent& event) override;
    void clearTracks();
    //[/UserMethods]

    void paint (Graphics& g) override;
    void resized() override;



private:
    //[UserVariables]   -- You can add your own custom variables in this section.
	std::vector<TrackPropertyPanel*> trackProperties;
	int offset = 0;
    ScopedPointer<DropShadow> dropShadow;
    ScopedPointer<DropShadower> dropShadower;
    ResizableEdgeComponent* hResizer;
    //[/UserVariables]

    //==============================================================================


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TrackPropertyView)
};

//[EndFile] You can add extra defines here...
//[/EndFile]

#endif   // __JUCE_HEADER_6CA99BC07CBFC596__
