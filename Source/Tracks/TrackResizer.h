/*
  ==============================================================================

    TrackResizer.h
    Created: 19 Dec 2016 12:53:31pm
    Author:  pueskma

  ==============================================================================
*/

#ifndef TRACKRESIZER_H_INCLUDED
#define TRACKRESIZER_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"

class TrackResizer : public ResizableEdgeComponent {

public:

	TrackResizer(Component* componentToResize,
		ComponentBoundsConstrainer* constrainer,
		Edge edgeToResize) : ResizableEdgeComponent(componentToResize,constrainer,edgeToResize) {

	}

	bool isShiftPressed() {
		return shiftPressed;
	}

	void mouseDown(const MouseEvent& e) override {
		if (e.mods.isShiftDown()) {
			shiftPressed = true;				 
		}
		else {
			shiftPressed = false;
		}

	}
private:
	bool shiftPressed = false;

};




#endif  // TRACKRESIZER_H_INCLUDED
