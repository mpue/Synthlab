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

//[Headers] You can add your own extra header files here...
#include "TrackNavigator.h"
#include "TrackPropertyPanel.h"
#include "../Project/Project.h"
#include "../Mixer.h"
//[/Headers]

#include "TrackPropertyView.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
TrackPropertyView::TrackPropertyView ()
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]


    //[UserPreSize]

    hResizer = new ResizableEdgeComponent(this, nullptr, ResizableEdgeComponent::rightEdge);
    //[/UserPreSize]

    setSize (600, 400);


    //[Constructor] You can add your own custom stuff here..
    // startTimer(70);

    dropShadow = new DropShadow(Colour::fromFloatRGBA(0,0,0,0.5),3,Point<int>(2,0));
    dropShadower = new DropShadower(*dropShadow);
    dropShadower->setOwner(this);

    Mixer::getInstance()->addChangeListener(this);
    addAndMakeVisible(hResizer);

    //[/Constructor]
}

TrackPropertyView::~TrackPropertyView()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]



    //[Destructor]. You can add your own custom destruction code here..

    for(std::vector<TrackPropertyPanel*>::iterator it = trackProperties.begin(); it != trackProperties.end();it++) {
        delete *it;
    }

    dropShadow = nullptr;
    dropShadower = nullptr;
    delete hResizer;
    //[/Destructor]
}

//==============================================================================
void TrackPropertyView::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.fillAll (Colour (0xff454545));

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void TrackPropertyView::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    //[UserResized] Add your own custom resize handling here..
    for(std::vector<TrackPropertyPanel*>::iterator it = trackProperties.begin(); it != trackProperties.end();it++) {
        (*it)->resized();
        (*it)->repaint();
    }

    int height = Project::DEFAULT_TRACK_HEIGHT;

    for (int i = 0; i < this->trackProperties.size();i++) {
        trackProperties.at(i)->setTopLeftPosition(trackProperties.at(i)->getX(), trackProperties.at(i)->getTrack()->getY());
        height += trackProperties.at(i)->getHeight();
    }

    setSize(getWidth(), height);

    hResizer->setBounds(getWidth() - 2, 0, 2, getHeight());

    //[/UserResized]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...

void TrackPropertyView::clearTracks() {
    for(std::vector<TrackPropertyPanel*>::iterator it = trackProperties.begin(); it != trackProperties.end();it++) {
        delete *it;
    }
    trackProperties.clear();
}

Track* TrackPropertyView::getTrack(int i) {
    return this->trackProperties.at(i)->getTrack();

}

void TrackPropertyView::setOffset(int offset)
{
	this->offset = offset;
	setTopLeftPosition(0, 75 + this->offset);

}

void TrackPropertyView::addTrack(Track* track)
{
    
	TrackPropertyPanel* panel = new TrackPropertyPanel();

    panel->setName(track->getName());

    Mixer::getInstance()->addChangeListener(panel);

	int yPos = 0;

	for (int i = 0; i < this->trackProperties.size();i++) {
		yPos += trackProperties.at(i)->getHeight();

	}

	panel->setBounds(0, yPos, 150, track->getHeight());
    panel->setTrack(track);
    panel->updateChannels();

    addAndMakeVisible(panel);
	trackProperties.push_back(panel);

	panel->addMouseListener(this, true);
    
    int height = 1;
    
    for (int i = 0; i < this->trackProperties.size();i++) {
        height += this->trackProperties.at(i)->getHeight();
    }

    setSize(getWidth(), height);
    hResizer->toFront(false);
}

void TrackPropertyView::changeListenerCallback (ChangeBroadcaster* source) {
    if (Mixer::getInstance() == source){
        
        int i = Mixer::getInstance()->getTracks().size() - 1;
        
        while (Mixer::getInstance()->getTracks().size() > trackProperties.size()) {
            addTrack(Mixer::getInstance()->getTracks().at(i--));
        }
        
        int height = 0;
        
        for (int i = 0; i < this->trackProperties.size();i++) {
            trackProperties.at(i)->setTopLeftPosition(trackProperties.at(i)->getX(), trackProperties.at(i)->getTrack()->getY());
            height += trackProperties.at(i)->getHeight();
        }
        
        setSize(getWidth(), height);
        
    }
    
}

void TrackPropertyView::timerCallback() {
    for (int i = 0;  i < this->trackProperties.size();i++) {
        this->trackProperties.at(i)->update();
    }
}


void TrackPropertyView::mouseDown(const MouseEvent& event) {

	if (event.mods.isLeftButtonDown()) {

		if (TrackPropertyPanel* t = dynamic_cast<TrackPropertyPanel*>(event.eventComponent)) {

			for (int i = 0; i < this->trackProperties.size();i++) {
				this->trackProperties.at(i)->setSelected(false);
			}

			t->setSelected(true);
		}

	}

}

//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Projucer information section --

    This is where the Projucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="TrackPropertyView" componentName=""
                 parentClasses="public Component, public ChangeListener, public Timer, public ComponentListener"
                 constructorParams="" variableInitialisers="" snapPixels="8" snapActive="1"
                 snapShown="1" overlayOpacity="0.330" fixedSize="0" initialWidth="600"
                 initialHeight="400">
  <BACKGROUND backgroundColour="ff454545"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
//[/EndFile]
