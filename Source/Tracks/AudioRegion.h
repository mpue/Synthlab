/*
  ==============================================================================

    Track.h
    Created: 25 Nov 2016 10:00:36pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#ifndef AUDIOREGION_H_INCLUDED 
#define AUDIOREGION_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"
#include "../MultiComponentDragger.h"
#include "../ResizeConstrainer.h"
#include "Region.h"


//==============================================================================
/*
*/
class AudioRegion : public Region
{
public:
    
    AudioRegion(AudioRegion* other, AudioFormatManager& manager, double sampleRate, long startSample, long numSamples);
    AudioRegion(AudioRegion* other, AudioFormatManager& manager, double sampleRate);
    AudioRegion(AudioSampleBuffer* source, AudioFormatManager& manager, long startSample, long sampleLength, double sampleRate);
    AudioRegion(File file, String refId, AudioFormatManager& manager,double sampleRate);
    ~AudioRegion();

    virtual void paint (Graphics&) override;
    virtual void resized() override;

    AudioThumbnail* getThumbnail();
    virtual void setThumbnailBounds(Rectangle<int>* bounds) override;
    virtual Rectangle<int>* getThumbnailBounds() override;
    
    virtual int getNumSamples() override;
    
    AudioSampleBuffer* getBuffer();
    const float* getReadBuffer(int channel);    

    virtual void setZoom(float zoom) override;
    virtual void setLoop(bool loop) override;
    
    // the timer is needed to update the thumbnail during record
    virtual void timerCallback() override;
    virtual void startRecording() override;
    virtual void stopRecording() override;
    
    // component dragger used to drag and drop regions
    void setDragger(MultiComponentDragger* dragger);
    
    virtual void updateThumb() override;
    virtual void componentMovedOrResized (Component& component,bool wasMoved, bool wasResized) override;

    // Reset des Resize-Gesture-States (resizeBaseSampleOffset). Wird beim
    // mouseUp aufgerufen, damit das naechste Linkstrim-Gesture wieder mit
    // einer frischen Basis startet — ohne Reset wuerde der zweite Linkstrim
    // gegen die Basis des ersten rechnen und springen.
    void mouseUp(const juce::MouseEvent& e) override;

    // Wird via addMouseListener auch fuer Events auf den Resizer-Kindern
    // gerufen. Wir nutzen das, um bei Resize-Gesture-Start einen Snapshot
    // fuer das Undo-System zu nehmen.
    void mouseDown(const juce::MouseEvent& e) override;

    // mouseDrag muss fuer Resizer-Events kurzgeschlossen werden, weil sonst
    // Region::mouseDrag dragger->handleMouseDrag aufruft und der Dragger nie
    // initialisiert wurde (mouseDown vom Resizer wurde uebersprungen) →
    // Null-Deref auf componentBeingDragged.
    void mouseDrag(const juce::MouseEvent& e) override;

    // ResizeState-Hooks (siehe Region.h): captureResizeState liefert die
    // drei Felder als Snapshot, applyResizeState schreibt sie zurueck und
    // synct die Bounds.
    Region::ResizeState captureResizeState() const override;
    void applyResizeState(const Region::ResizeState& state) override;
    
    void setClipRefId(String id);
    String getClipRefid();

    // Buffer-Mutation-Tracking: getrennt vom thumbnail-orientierten 'dirty'-
    // Flag. Wird auf true gesetzt sobald eine destruktive Operation den
    // audioBuffer veraendert (Normalize, Fade, Reverse, Consolidate, Pitch ...).
    // Beim naechsten Project::save persistiert TrackNavigator::persistDirtyAudioRegions
    // den aktuellen Buffer als neue WAV in $project/edits/ und vergibt einen
    // neuen clipRefId — danach wird das Flag zurueckgesetzt. Reload findet die
    // edited-Version unter neuem refId in _audio.xml.
    void markBufferMutated() { bufferMutated = true; }
    void markBufferClean()   { bufferMutated = false; }
    bool isBufferMutated() const { return bufferMutated; }

    // Tempo-Follow: Reference-Tempo ist das BPM bei dem dieser audioBuffer
    // ursprünglich aufgenommen / importiert wurde. Wenn followTempo=true
    // und Project::setTempo das BPM aendert, stretcht TrackNavigator den
    // Buffer mit Rubberband um den Faktor referenceTempo / currentTempo und
    // markiert die Region als bufferMutated. Referenz bleibt nach Stretch
    // unveraendert — so wird auch 120 → 100 → 130 immer relativ zum
    // urspruenglichen Reference berechnet, keine Stretch-Fehlerakkumulation.
    double getReferenceTempo() const  { return referenceTempo; }
    void   setReferenceTempo(double t);
    bool   getFollowTempo() const     { return followTempo; }
    void   setFollowTempo(bool b);

    // Wendet einen Tempo-Stretch mit Rubberband direkt auf den audioBuffer
    // an. Ratio = referenceTempo / targetTempo (>1 = laenger/langsamer,
    // <1 = kuerzer/schneller). Markiert die Region anschliessend als
    // bufferMutated. No-op wenn followTempo=false, referenceTempo<=0 oder
    // ratio ~ 1.0. Sample-Offsets/Width werden mit-angepasst, damit die
    // Region in der Timeline an derselben Stelle bleibt aber die neue Laenge
    // ausfuellt.
    void applyTempoStretch(double targetTempo, double sampleRate);
       
    float getPitch();
    void setPitch(float pitch);

    // Per-region gain multiplier (1.0 = unity, 0.0 = silence, 2.0 = +6 dB).
    // Applied at playback time in Track::getSample; the underlying buffer
    // stays untouched, so undo just restores the old multiplier. Persisted
    // by TrackNavigator under the "gain" property; SetGainForRegionAction
    // is the canonical undoable mutation path.
    float getGain() const override { return gain; }
    void  setGain (float g) override;

    // Load-only Setter: setzt das Pitch-State-Member OHNE Rubberband-Shift.
    // Wird beim Projekt-Open verwendet, weil der geladene WAV bereits in
    // diesem Pitch-State steckt — ein erneutes setPitch wuerde den Shift
    // ein zweites Mal anwenden.
    void adoptLoadedPitch(float pitchSemitones);

    virtual juce::Array<juce::PropertyComponent*>& getProperties() override;
    virtual void createProperties() override;

    void resizeAudio(int length);

    void quantize(int quantizeGridSamples) override {
		// empty implementation, audio regions are not quantized
    }

    void consolidate() override;

    // Sample-Offset INNERHALB des Audio-Buffers, an dem die linke Kante der
    // Region beginnt. Wird beim Links-Trim erhoeht; getNumSamples() bleibt
    // die sichtbare (bzw. spielende) Laenge ab diesem Offset. consolidate()
    // baked das in den Buffer ein und setzt zurueck auf 0.
    long getSourceStartSample() const override { return sourceStartSample; }
    void setSourceStartSample(long s) { sourceStartSample = juce::jmax(0L, s); }

    // Sample-akkurate Trim-Laenge — vom Resize-Handler/Consolidate/Load
    // direkt gesetzt, damit der Save-Pfad nicht ueber Pixel-Breite zurueck-
    // rechnen muss (das verlor pro Round-Trip bis zu eine halbe Pixelbreite
    // an Samples, bei niedrigem Zoom mehrere ms pro Speicherung).
    // 0 = "nie explizit gesetzt" -> Save faellt auf die alte Pixel-Math zurueck.
    long getTrimmedNumSamples() const { return trimmedNumSamples; }
    void setTrimmedNumSamples(long s) { trimmedNumSamples = juce::jmax(0L, s); }

    // PropertyView "Length"-Edit. Klemmt gegen Buffer-Ende und Nachbar-Region
    // (clampLengthForRegion) und stempelt Bounds/trimmedNumSamples sample-
    // genau. Sehr aehnlich zum Right-Edge-Resize, aber ohne Constrainer- und
    // Drag-Pfade.
    void setLengthSamples(long n) override;

private:

    struct PitchListener : juce::Value::Listener
    {
        PitchListener(juce::Value& v, AudioRegion* r) : region(r), value(v) { value.addListener(this); }
        ~PitchListener() {}  // no need to remove the listener

        void valueChanged(juce::Value& value) override {
            region->setPitch(value.toString().getFloatValue());
        }
        AudioRegion* region;
        juce::Value value;
    };

    struct ReferenceTempoListener : juce::Value::Listener
    {
        ReferenceTempoListener(juce::Value& v, AudioRegion* r) : region(r), value(v) { value.addListener(this); }
        ~ReferenceTempoListener() {}

        void valueChanged(juce::Value& value) override {
            region->setReferenceTempo(value.toString().getDoubleValue());
        }
        AudioRegion* region;
        juce::Value value;
    };

    struct FollowTempoListener : juce::Value::Listener
    {
        FollowTempoListener(juce::Value& v, AudioRegion* r) : region(r), value(v) { value.addListener(this); }
        ~FollowTempoListener() {}

        void valueChanged(juce::Value& value) override;
        AudioRegion* region;
        juce::Value value;
    };

    // Bridges the property-panel slider to setGain. Slider edits are NOT
    // routed through SetGainForRegionAction — that matches the existing
    // pitch/refTempo convention in this file, where property edits mutate
    // the region directly. Undo of slider edits is therefore not supported;
    // scripts / future actions are the canonical undoable path.
    struct GainListener : juce::Value::Listener
    {
        GainListener(juce::Value& v, AudioRegion* r) : region(r), value(v) { value.addListener(this); }
        ~GainListener() {}

        void valueChanged(juce::Value& value) override {
            region->setGain(value.toString().getFloatValue());
        }
        AudioRegion* region;
        juce::Value value;
    };


    AudioSampleBuffer* audioBuffer;
    AudioSampleBuffer* audioBufferProcessed = nullptr;
    long sourceStartSample = 0;
    // Sample-akkurate Trim-Laenge. Wird in den Konstruktoren auf die initiale
    // Region-Laenge gesetzt und vom Resize-Handler/Consolidate aktualisiert.
    // Source-of-truth fuer Save (TrackNavigator::getConfiguration).
    long trimmedNumSamples = 0;
    bool bufferMutated = false;

    // Default 120; in den Konstruktoren wird der Wert via Project::getTempo()
    // auf das aktuelle Projekt-Tempo gesetzt (= "diese Aufnahme war auf X BPM").
    double referenceTempo = 120.0;
    bool   followTempo    = false;
    float  gain           = 1.0f;

    // Resize-Gesture-Basis: speichern beim ersten Tick (sentinel = -1) und
    // sourceStartSample im laufenden Resize absolut daraus berechnen, statt
    // pro Tick inkrementell aufzuaddieren. Vermeidet Rundungs-Akkumulation,
    // die sonst beim Linkstrim einer rechts-platzierten Region den Inhalt
    // langsam nach links scrollen laesst. Reset auf -1 in mouseUp am Resizer.
    long resizeBaseSampleOffset = -1;
    long resizeBaseSourceStartSample = 0;

    // Resize-Undo-Snapshot. Wird in mouseDown auf einem Resizer-Child
    // gesetzt (gestureStateValid=true) und in mouseUp ausgewertet, um
    // genau eine ResizeRegionAction pro Gesture zu pushen.
    Region::ResizeState resizeGestureStart;
    bool resizeGestureStateValid = false;
    AudioThumbnail* thumbnail;
    AudioThumbnailCache* thumbnailCache;
    ScopedPointer<AudioFormatReaderSource> readerSource;
    ResizeConstrainer* constrainer;

    double getQuarterNoteLength();
    long snapToQuarterNote(long samplePosition);
    void updateSnapSettings();

    Rectangle<int>* thumbnailBounds = NULL;
    virtual void changeListenerCallback(ChangeBroadcaster * source) override;
    
    // reference to original file
    String clipRefId;
    // Aktueller effektiver Pitch-Shift in Halbtoenen, relativ zum nativen
    // Source-Pitch. 0 = unveraendert. Die Buffer-Daten sind bereits auf
    // diesen Wert geshiftet — setPitch berechnet die Differenz und macht
    // nur den Delta-Stretch. Beim Load matched member-Pitch automatisch
    // zum geladenen WAV (kein Re-Apply).
    float pitch = 0.0f;

    PitchListener* pitchListener;
    juce::Value* pitchValue;
    juce::PropertyComponent* pitchProp;

    ReferenceTempoListener* refTempoListener = nullptr;
    juce::Value* refTempoValue = nullptr;
    juce::PropertyComponent* refTempoProp = nullptr;

    FollowTempoListener* followTempoListener = nullptr;
    juce::Value* followTempoValue = nullptr;
    juce::PropertyComponent* followTempoProp = nullptr;

    GainListener* gainListener = nullptr;
    juce::Value* gainValue = nullptr;
    juce::PropertyComponent* gainProp = nullptr;

    float* tempBufferLeft = nullptr;
    float* tempBufferRight = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioRegion)
};


#endif  // AUDIOREGION_H_INCLUDED
