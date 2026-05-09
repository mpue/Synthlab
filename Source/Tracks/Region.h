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
#include "../MultiComponentDragger.h"
#include "../Project/Project.h"

class Region : public Component,  public ChangeListener, public ChangeBroadcaster, public ComponentListener, public Timer {
    
public:

    Region();
    virtual ~Region();
    
    virtual void paint (Graphics&) override = 0;
    virtual void resized() override = 0;

    virtual void setZoom(float zoom);
    float getZoom() const { return zoom; }

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
    // quantize=true (default) rundet sampleOffset auf bufferSize-Vielfaches —
    // sinnvoll fuer Audio-Pfade, fatal fuer Resize-Loops: deltaLeft (=
    // snappedLeft - getSampleOffset()) treibt sonst pro Tick um den Quantize-
    // Verlust nach oben, sourceStartSample drifted, der Inhalt scrollt
    // langsam nach links. Resize-Caller geben quantize=false.
    virtual void setSampleOffset(long offset, bool reminder, bool notify, bool quantize = true);

    // Sample-Offset in den Quell-Daten (Audio- bzw. MIDI-Buffer-Zeit), an dem
    // die linke Kante der Region beginnt. Default 0 = konsolidiert (sicht-
    // barer Bereich startet bei Buffer-Position 0). AudioRegion und MidiRegion
    // ueberschreiben fuer Resize-vorne-Persistenz.
    virtual long getSourceStartSample() const { return 0; }

    // Wenn external (Track::duplicateRegion / pasteRegion etc.) groessere
    // Bounds-Updates macht, soll der Subclass-componentMovedOrResized-Handler
    // diese als "kein User-Edit" behandeln und nichts neu berechnen. Selbe
    // Semantik wie das intern fuer setZoom benutzte Flag.
    void setBoundsUpdateInProgress(bool b) { zoomInProgress = b; }
    
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

    // Consolidate: bake all loop repetitions into the region data and
    // reset loopCount to 0 so the region plays as a single flat clip.
    virtual void consolidate() {}
    
    // component dragger used to drag and drop regions
    void setDragger(MultiComponentDragger* dragger);
    
    virtual void updateThumb() = 0;    
    virtual void componentMovedOrResized (Component& component,bool wasMoved, bool wasResized) override = 0;
    
    virtual juce::Array<juce::PropertyComponent*>& getProperties();
    virtual void createProperties();

    void setDirty(bool dirty);
    virtual void quantize(int quantizeGridSamples) = 0;

protected:
    
    struct NameListener : juce::Value::Listener
    {
        NameListener(juce::Value& v, Region* r) : region(r), value(v) { value.addListener(this); }
        ~NameListener() {}  // no need to remove the listener

        void valueChanged(juce::Value& value) override {
            region->setName(value.toString());
        }
        Region* region;
        juce::Value value;
    };

    bool dirty;
    long dragStartSampleOffset = 0;

    // True while setZoom is mid-flight. Subclass componentMovedOrResized
    // implementations must skip recomputing sampleOffset from pixel coords
    // in that case — otherwise rounding/snap drift in the zoom-induced move
    // shifts the region (and any recorded MIDI timestamps) by a sample or a
    // whole grid step on every zoom change.
    bool zoomInProgress = false;

    float zoom = 20;
    String name;
    int offset = 0;
    double sampleRate = Project::getInstance()->getSampleRate();
    long sampleOffset = 0;
    long oldOffset = 0;
    int loopCount = 1;
    bool selected = false;
    bool loop = false;
    virtual void changeListenerCallback(ChangeBroadcaster* source) override {};

    ResizableEdgeComponent* resizerR = nullptr;
    ResizableEdgeComponent* resizerL = nullptr;
    
    MultiComponentDragger* dragger = NULL;

    NameListener* nameListener;
    juce::Value* nameValue;

    juce::PropertyComponent* nameProp;
    juce::Array<juce::PropertyComponent*> properties;
   
};


#endif /* Region_h */
