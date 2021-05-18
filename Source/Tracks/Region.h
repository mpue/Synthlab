//
//  Region.h
//  WaveX
//
//  Created by Matthias Pueski on 28.12.16.
//
//

#ifndef Region_h
#define Region_h

#include "../JuceLibraryCode/JuceHeader.h"
#include "MultiComponentDragger.h"

class Region : public Component,  public ChangeListener, public ChangeBroadcaster, public ComponentListener, public Timer {
    
public:

    Region();
    virtual ~Region();
    
    virtual void paint (Graphics&) override = 0;
    virtual void resized() override = 0;

    virtual void setZoom(float zoom);

    void setName(String name);
    String getName();

    virtual int getNumSamples() = 0;
    virtual void setThumbnailBounds(Rectangle<int>* bounds) = 0;
    virtual Rectangle<int>* getThumbnailBounds() = 0;
    void setSelected(bool selected);
    bool isSelected();
    
    virtual void setLoop(bool loop);
    bool isLoop();
    
    void setOffset(int offset);
    int getOffset();
    
    virtual void setLoopCount(int count);
    int getLoopCount();
    
    long getSampleOffset();
    void setSampleOffset(long offset,bool reminder, bool notify);
    
    long getOldOffset();
    
    double getSampleRate();
    
    void mouseDown(const MouseEvent & e) override;
    void mouseUp(const MouseEvent & e) override;
    void mouseDrag(const MouseEvent & e) override;
    
    // the timer is needed to updat the thumbnail during record
    virtual void timerCallback() override;
    
    virtual void startRecording() = 0;
    virtual void stopRecording() = 0;
    
    virtual void reset() {
    }
    
    // component dragger used to drag and drop regions
    void setDragger(MultiComponentDragger* dragger);
    
    virtual void updateThumb() = 0;
    
    virtual void componentMovedOrResized (Component& component,bool wasMoved, bool wasResized) override = 0;
    
protected:
    
    float zoom = 30;
    String name;
    int offset = 0;
    double sampleRate = 48000;
    long sampleOffset = 0;
    long oldOffset = 0;
    int loopCount = 1;
    bool selected = false;
    bool loop = false;
    virtual void changeListenerCallback(ChangeBroadcaster * source) override = 0;

    ResizableEdgeComponent* resizerR;
    ResizableEdgeComponent* resizerL;
    
    MultiComponentDragger* dragger = NULL;
    
    
};


#endif /* Region_h */
