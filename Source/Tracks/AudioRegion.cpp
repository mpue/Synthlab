/*
  ==============================================================================

    Track.cpp
    Created: 25 Nov 2016 10:00:36pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "AudioRegion.h"
#include "Track.h"
#include "../Project/Project.h"
#include "../AudioEngine/SmartSample.h"

AudioRegion::AudioRegion(AudioRegion* other, AudioFormatManager& manager, double sampleRate, long startSample, long numSamples) {

    this->sampleRate = sampleRate;
    this->zoom = other->zoom;
    this->constrainer = new ResizeConstrainer(this->zoom / 4);
    
    resizerR = new ResizableEdgeComponent(this,constrainer,ResizableEdgeComponent::rightEdge);
    resizerL = new ResizableEdgeComponent(this,constrainer,ResizableEdgeComponent::leftEdge);
    
    this->thumbnailCache = new AudioThumbnailCache(1);
    this->thumbnail = new AudioThumbnail(512, manager, *this->thumbnailCache);
    this->thumbnail->addChangeListener(this);
    
    audioBuffer = new AudioSampleBuffer(2, numSamples);

    if (other->getBuffer()->getNumChannels() == 1) {
        audioBuffer->copyFrom(0, 0, *other->getBuffer(), 0, startSample, numSamples);
        audioBuffer->copyFrom(1, 0, *other->getBuffer(), 0, startSample, numSamples);
    }
    else {
        audioBuffer->copyFrom(0, 0, *other->getBuffer(), 0, startSample, numSamples);
        audioBuffer->copyFrom(1, 0, *other->getBuffer(), 1, startSample, numSamples);
    }

    this->thumbnail->reset(2, sampleRate);
    this->thumbnail->addBlock(0, *audioBuffer, 0, numSamples);
    
    this->name = other->getName();
    
    // KORREKTUR: Verwende numSamples statt other->getBuffer()->getNumSamples()
    // da wir nur einen Teil des Buffers kopiert haben
    setSize((numSamples / sampleRate) * this->zoom, Project::DEFAULT_TRACK_HEIGHT - 25);

    this->offset = 0;

    addAndMakeVisible(resizerL);
    addAndMakeVisible(resizerR);

    // mouseUp am Resizer triggert AudioRegion::mouseUp -> Resize-Basis reset.
    if (resizerL != nullptr) resizerL->addMouseListener(this, false);
    if (resizerR != nullptr) resizerR->addMouseListener(this, false);

    addComponentListener(this);
    dirty = false;
    setPitch(1);
    createProperties();
    updateSnapSettings();
}


AudioRegion::AudioRegion(AudioSampleBuffer* source, AudioFormatManager& manager, long startSample, long sampleLength, double sampleRate) {

    this->zoom = Project::getInstance()->getTempo() / 8;
    this->constrainer = new ResizeConstrainer(this->zoom / 4);
    
    resizerR = new ResizableEdgeComponent(this,constrainer,ResizableEdgeComponent::rightEdge);
    resizerL = new ResizableEdgeComponent(this,constrainer,ResizableEdgeComponent::leftEdge);
    
    this->sampleRate = sampleRate;
    
    this->thumbnailCache = new AudioThumbnailCache(1);
    this->thumbnail = new AudioThumbnail(512, manager, *this->thumbnailCache);
    this->thumbnail->addChangeListener(this);
    
    audioBuffer = new AudioSampleBuffer(2, sampleLength);

    if (source != nullptr) {

        if (source->getNumChannels() == 1) {
            audioBuffer->copyFrom(0, 0, *source, 0, 0, sampleLength);
            audioBuffer->copyFrom(1, 0, *source, 0, 0, sampleLength);
        }
        else {
            audioBuffer->copyFrom(0, 0, *source, 0, 0, sampleLength);
            audioBuffer->copyFrom(1, 0, *source, 1, 0, sampleLength);
        }
    }
    else {
        // Recording case: AudioSampleBuffer leaves memory uninitialised, so
        // without this clear the new region shows whatever heap garbage was
        // previously freed at that address — frequently the prior recording's
        // buffer, which made it look like the old audio "leaked" into the
        // new region.
        audioBuffer->clear();
    }
    
    this->thumbnail->reset(2, sampleRate);
    this->thumbnail->addBlock(0, *audioBuffer, 0, sampleLength);
    
    this->name = "new";
    this->offset = 0;
        
    setSize((audioBuffer->getNumSamples() / sampleRate) * this->zoom, Project::DEFAULT_TRACK_HEIGHT);

    addAndMakeVisible(resizerL);
    addAndMakeVisible(resizerR);
    if (resizerL != nullptr) resizerL->addMouseListener(this, false);
    if (resizerR != nullptr) resizerR->addMouseListener(this, false);
    addComponentListener(this);

    dirty = false;
    // setPitch(1);
    createProperties();
    updateSnapSettings();
}

AudioRegion::AudioRegion(AudioRegion* other, AudioFormatManager& manager, double sampleRate) {
    
    this->zoom = other->zoom;
    this->constrainer = new ResizeConstrainer(this->zoom / 4);
    
    resizerR = new ResizableEdgeComponent(this,constrainer,ResizableEdgeComponent::rightEdge);
    resizerL = new ResizableEdgeComponent(this,constrainer,ResizableEdgeComponent::leftEdge);
    
    this->sampleRate = sampleRate;
    
    this->thumbnailCache = new AudioThumbnailCache(1);
    this->thumbnail = new AudioThumbnail(512, manager, *this->thumbnailCache);
    this->thumbnail->addChangeListener(this);
    this->setThumbnailBounds(other->getThumbnailBounds());
    
    audioBuffer = new AudioSampleBuffer(2, other->getBuffer()->getNumSamples());

    if (other->getBuffer()->getNumChannels() == 1) {
        audioBuffer->copyFrom(0, 0, *other->getBuffer(), 0, 0, other->getBuffer()->getNumSamples());
        audioBuffer->copyFrom(1, 0, *other->getBuffer(), 0, 0, other->getBuffer()->getNumSamples());
    }
    else {
        audioBuffer->copyFrom(0, 0, *other->getBuffer(), 0, 0, other->getBuffer()->getNumSamples());
        audioBuffer->copyFrom(1, 0, *other->getBuffer(), 1, 0, other->getBuffer()->getNumSamples());
    }
        
    this->thumbnail->reset(2, sampleRate);
    this->thumbnail->addBlock(0, *audioBuffer, 0,other->getBuffer()->getNumSamples());
    
    this->name = other->getName();
    
    /*
    double length = this->thumbnail->getTotalLength();
    setSize(length * this->zoom, other->getHeight());
     */
    
    setSize((other->getBuffer()->getNumSamples() / sampleRate) * this->zoom, Project::DEFAULT_TRACK_HEIGHT);

    this->offset = 0;

    addAndMakeVisible(resizerL);
    addAndMakeVisible(resizerR);
    if (resizerL != nullptr) resizerL->addMouseListener(this, false);
    if (resizerR != nullptr) resizerR->addMouseListener(this, false);

    addComponentListener(this);

    dirty = false;

    setPitch(1);
    createProperties();
    updateSnapSettings();
}

//==============================================================================
AudioRegion::AudioRegion(File file, String refId, AudioFormatManager& manager, double sampleRate)
{
    this->zoom = 40;
    this->constrainer = new ResizeConstrainer(this->zoom / 4);
    resizerR = new ResizableEdgeComponent(this, constrainer, ResizableEdgeComponent::rightEdge);
    resizerL = new ResizableEdgeComponent(this, constrainer, ResizableEdgeComponent::leftEdge);
    this->sampleRate = sampleRate;

    SmartSample* smartSample = new SmartSample();
    smartSample->loadFromFile(file, sampleRate);

    int numChannels = smartSample->getBuffer().getNumChannels();
    int lengthInSamples = smartSample->getLengthInSamples();

    this->thumbnailCache = new AudioThumbnailCache(5);
    this->thumbnail = new AudioThumbnail(512, manager, *this->thumbnailCache);
    this->thumbnail->addChangeListener(this);
    this->thumbnail->reset(numChannels, sampleRate);

    audioBuffer = new AudioSampleBuffer(smartSample->getBuffer());

    // KORRIGIERT: addBlock mit korrektem Buffer-Offset (immer 0, da wir den ganzen Buffer �bergeben)
    this->thumbnail->addBlock(0, *audioBuffer, 0, audioBuffer->getNumSamples());

    this->name = file.getFileNameWithoutExtension();
    this->clipRefId = refId;
    setSize((lengthInSamples / sampleRate) * this->zoom, Project::DEFAULT_TRACK_HEIGHT);

    Logger::getCurrentLogger()->writeToLog(getBounds().toString());

    this->offset = 0;

    addComponentListener(this);
    addAndMakeVisible(resizerR);
    addAndMakeVisible(resizerL);

    dirty = false;
    setPitch(1);
    createProperties();
    updateSnapSettings();

    // setInterceptsMouseClicks(false, true);

    delete smartSample;
}

AudioRegion::~AudioRegion()
{
    if (pitchValue)
        pitchValue->removeListener(pitchListener);
    delete pitchListener;
    delete pitchValue;

    delete[] tempBufferLeft;
    delete[] tempBufferRight;

    delete this->thumbnailBounds;
    delete this->thumbnailCache;
    delete this->thumbnail;
    delete this->audioBuffer;
    delete this->audioBufferProcessed;
    // resizerR and resizerL are deleted by Region::~Region()
    delete this->constrainer;
}

void AudioRegion::timerCallback() {
    if (dirty)
        updateThumb();
}

void AudioRegion::updateThumb()
{
    // double length = this->thumbnail->getTotalLength();
    this->thumbnail->reset(2, sampleRate);
    this->thumbnail->addBlock(0, *audioBuffer, 0, audioBuffer->getNumSamples());
    double length = thumbnail->getTotalLength();
    setSize(length * this->zoom, getHeight());
    this->thumbnailBounds->setWidth(length * this->zoom);
    repaint();
    dirty = false;
}


void AudioRegion::setDragger(MultiComponentDragger *dragger) {
    this->dragger = dragger;
}

void AudioRegion::startRecording() {
    getThumbnail()->reset(2, Project::getInstance()->getSampleRate());
    startTimer(100);
}

void AudioRegion::stopRecording() {
    stopTimer();
}

void AudioRegion::consolidate()
{
    int numChannels = audioBuffer->getNumChannels();

    if (loop && loopCount > 0)
    {
        // Looping case: repeat the audio buffer (loopCount + 1) times.
        // Call updateThumb() afterwards so the component expands to the full length.
        int oneLoopSamples = audioBuffer->getNumSamples();
        int totalSamples   = oneLoopSamples * (loopCount + 1);

        auto* newBuffer = new AudioSampleBuffer(numChannels, totalSamples);

        for (int i = 0; i <= loopCount; i++)
            for (int ch = 0; ch < numChannels; ch++)
                newBuffer->copyFrom(ch, i * oneLoopSamples, *audioBuffer, ch, 0, oneLoopSamples);

        delete audioBuffer;
        audioBuffer = newBuffer;

        loopCount = 0;
        loop      = false;
        dirty     = true;
        updateThumb();
    }
    else
    {
        // Konsolidierter Bereich = [sourceStartSample, sourceStartSample + numSamples).
        // Alles davor / danach wird endgueltig aus dem audioBuffer entfernt,
        // sourceStartSample faellt auf 0. Reine Right-Trim-Faelle haben
        // sourceStartSample == 0, dann ist es ein normaler Trim am Ende.
        const long visibleLen = (long) std::round((double) getWidth() / zoom * sampleRate);
        const long sss        = sourceStartSample;
        if (visibleLen <= 0) return;

        // Out-of-bounds-Schutz: visibleLen darf nicht ueber den Buffer
        // hinauslaufen (kann passieren wenn die Region durch Resize-zurueck
        // groesser wurde als die Quelle).
        const long maxAvailable = (long) audioBuffer->getNumSamples() - sss;
        const long copyLen = juce::jlimit(0L, maxAvailable, visibleLen);
        if (copyLen <= 0) return;

        auto* newBuffer = new AudioSampleBuffer(numChannels, (int) visibleLen);
        newBuffer->clear();
        for (int ch = 0; ch < numChannels; ch++)
            newBuffer->copyFrom(ch, 0, *audioBuffer, ch, (int) sss, (int) copyLen);
        delete audioBuffer;
        audioBuffer = newBuffer;
        sourceStartSample = 0;

        // Thumbnail neu aus dem getrimmten Buffer aufbauen.
        this->thumbnail->reset(numChannels, sampleRate);
        this->thumbnail->addBlock(0, *audioBuffer, 0, (int) visibleLen);
        if (this->thumbnailBounds != nullptr)
            this->thumbnailBounds->setWidth(getWidth());
        dirty = true;
        repaint();
    }
}

void AudioRegion::setLoop(bool loop)
{
    if (loop) {
         setSize(this->thumbnail->getTotalLength() * (loopCount + 1) * this->zoom, getHeight());
    }
    else {
         setSize(this->thumbnail->getTotalLength() * this->zoom, getHeight());
    }
    
    this->loop = loop;
    dirty = true;
}

const float* AudioRegion::getReadBuffer(int channel) {
    return audioBuffer->getReadPointer(channel);
}

AudioSampleBuffer* AudioRegion::getBuffer() {
    return audioBuffer;
}

void AudioRegion::setZoom(float zoom) {
    // Sichtbare Sample-Laenge JETZT festhalten — solange this->zoom noch den
    // alten Wert hat, liefert getNumSamples() den korrekten getrimmten Stand.
    // Erst danach den neuen Zoom uebernehmen, sonst rechnet getNumSamples()
    // die Pixel-Breite mit dem neuen Zoom zurueck und der Trim wird verfaelscht.
    const long visibleSamples = (long) getNumSamples();

    this->zoom = zoom;

    zoomInProgress = true;
    if (loop) {
        setSize((int) std::round((double) visibleSamples / sampleRate * (loopCount + 1) * this->zoom), getHeight());
    }
    else {
        setSize((int) std::round((double) visibleSamples / sampleRate * this->zoom), getHeight());
    }

    // Snap auf Viertelnoten anwenden
    long snappedOffset = snapToQuarterNote(this->sampleOffset);
    int newOffset = (snappedOffset / this->sampleRate) * zoom;
    this->setOffset(newOffset);
    this->setTopLeftPosition(newOffset, 0);
    zoomInProgress = false;

    // Resize-Handle-Breite: 5 px war zu schmal — Nutzer haben beim Versuch,
    // links zu trimmen, oft daneben getroffen und dann den Region-Body
    // gegriffen, sodass aus dem Trim ein Move-Drag wurde. 8 px gibt
    // brauchbare Hit-Area, ohne den Body zu verdecken.
    resizerR->setSize(8, getHeight());
    resizerR->setTopLeftPosition(getWidth() - 8, 0);
    if (resizerL != nullptr) {
        resizerL->setSize(8, getHeight());
        resizerL->setTopLeftPosition(0, 0);
    }

    // thumbnailBounds.width muss dem VOLLEN Buffer-Extent entsprechen, sonst
    // verliert getNumSamples() seine Trim-Detektion: die vergleicht
    // getWidth() (sichtbar) mit thumbnailBounds.width (voll). Setzt man die
    // hier auf getWidth(), sind beide gleich und die Region gilt naechstes
    // Mal als "nicht getrimmt" — sie springt auf die Original-Laenge zurueck.
    const int fullWidth = (int) std::round((double) audioBuffer->getNumSamples() / sampleRate * this->zoom);
    this->thumbnailBounds->setSize(fullWidth, getHeight());

    // Aktualisiere Snap-Einstellungen f�r Viertelnoten
    updateSnapSettings();

    constrainer->setMaxWidth((audioBuffer->getNumSamples() / sampleRate) * this->zoom);

    dirty = true;
}

void AudioRegion::setThumbnailBounds(Rectangle<int>* bounds) {
    this->thumbnailBounds = bounds;
}

Rectangle<int>*  AudioRegion::getThumbnailBounds() {
    return new Rectangle<int>(*this->thumbnailBounds);
};

void AudioRegion::paint (Graphics& g)
{
    Rectangle<int> b = *this->thumbnailBounds;
    int effectiveWidth = getWidth() ;//(this->getNumSamples() / sampleRate) * this->zoom;

    // Track-Farbe als Region-Body — so erkennt man auf einen Blick, zu welchem
    // Track die Region gehört, auch wenn sie hin- und hergeschoben wurde.
    juce::Colour bodyColour(0xff4682b4); // steelblue als Fallback
    if (auto* parentTrack = dynamic_cast<Track*>(getParentComponent()))
        bodyColour = parentTrack->getTrackColour();

    const bool sel = (dragger != nullptr) && dragger->isSelected(this);
    const juce::Colour fill   = sel ? bodyColour.brighter(0.25f) : bodyColour;
    const juce::Colour stroke = bodyColour.darker(0.5f);
    const juce::Colour textCol = bodyColour.contrasting(0.85f);

    g.setColour(fill);
    g.fillRoundedRectangle(b.getX(), b.getY(), effectiveWidth, b.getHeight(), 10);

    // Wellenform in halbtransparentem Weiß — auf jedem Hue gut lesbar.
    // Beim Links-Trim verschiebt sich der sichtbare Slice im Buffer von
    // sourceStartSample bis sourceStartSample + numSamples; das Thumbnail
    // muss diesen Slice in den effektiven (getrimmten) Pixel-Bereich
    // zeichnen, sonst sieht es so aus, als wuerde die Region nur verschoben,
    // obwohl der Audio-Pfad korrekt von einer spaeteren Position liest.
    g.setColour(juce::Colours::white.withAlpha(0.85f));
    const double srForPaint = sampleRate > 0.0 ? sampleRate : 44100.0;
    const double visibleSeconds = (double) getNumSamples() / srForPaint;
    const double startSeconds   = (double) sourceStartSample / srForPaint;
    const double endSeconds     = startSeconds + visibleSeconds;
    const juce::Rectangle<int> waveArea(b.getX(), b.getY(), effectiveWidth, b.getHeight());
    this->thumbnail->drawChannels(g, waveArea, startSeconds, endSeconds, 1.0f);

    g.setColour(stroke);
    g.drawRoundedRectangle(b.getX(), b.getY(), effectiveWidth, b.getHeight(), 10, 1.0f);

    g.setColour(textCol);
    g.setFont(14.0);
    g.drawText(name, 10, 10, 140, 20, juce::Justification::left);
    g.drawText(juce::String(sampleOffset), 10, 30, 140, 20, juce::Justification::left);

    if (loop) {
        for (int i = 1; i <= loopCount; i++) {
            g.setColour(fill);
            Rectangle<int> loopBounds(b.getX() + i * effectiveWidth / (loopCount + 1), b.getY(), effectiveWidth, b.getHeight());
            g.fillRoundedRectangle(b.getX() + i * effectiveWidth / (loopCount + 1), b.getY(), effectiveWidth, b.getHeight(), 10);
            g.setColour(juce::Colours::white.withAlpha(0.6f));

            // Auch in den Loop-Wiederholungen den getrimmten Slice zeichnen.
            this->thumbnail->drawChannels(g, loopBounds, startSeconds, endSeconds, 1.0f);
        }
    }
    dirty = false;
}

void AudioRegion::resized()
{
    Logger::getCurrentLogger()->writeToLog(String(this->thumbnail->getTotalLength()));
    if (this->thumbnailBounds != NULL) {
        this->thumbnailBounds->setHeight(getHeight());
        // this->thumbnailBounds->setWidth(getWidth());
    }
    
    constrainer->setRaster(zoom / 4);
    int maxWidth = (audioBuffer->getNumSamples() / sampleRate) * this->zoom;
    constrainer->setMaxWidth(maxWidth);
    resizerR->setTopLeftPosition(getWidth() - 8, 0);
    if (resizerL != nullptr) resizerL->setTopLeftPosition(0, 0);
    dirty = true;
}

AudioThumbnail* AudioRegion::getThumbnail() {
    
    return this->thumbnail;
}

void AudioRegion::changeListenerCallback(ChangeBroadcaster * source)
{
    if (source == this->thumbnail) {
        dirty = true;
        
    }
}

int AudioRegion::getNumSamples() {
    
    int originalSize = thumbnailBounds->getWidth();
    int reducedSize = getWidth();
    
    //cout << "Original : " << originalSize << " reduced : " << reducedSize << endl;
    
    // region has been resized with resizer component, thus we need to report another sample size
    if (reducedSize > 0 && reducedSize  < originalSize) {
        int effectiveSamples = (getWidth() / zoom) * sampleRate;
        
        // std::cout << effectiveSamples << std::endl;
        return effectiveSamples;
    }
    
    
    return audioBuffer->getNumSamples();
}

void AudioRegion::mouseUp(const juce::MouseEvent& /*e*/) {
    // Resize-Gesture-Basis verwerfen, damit der naechste Linkstrim eine
    // frische Basis aus dem dann aktuellen sampleOffset/sourceStartSample
    // nimmt. Wird via addMouseListener auch ausgeloest wenn die Maus auf
    // einem der Resizer (Child) losgelassen wird.
    resizeBaseSampleOffset = -1;
}

void AudioRegion::componentMovedOrResized(Component& component, bool wasMoved, bool wasResized) {

    // Skip when the move/resize was triggered by setZoom — recomputing
    // sampleOffset from the new pixel position would drift it (rounding +
    // snap-to-quarter), shifting the region on every zoom change.
    if (zoomInProgress)
        return;

    Track* parentTrack = dynamic_cast<Track*>(getParentComponent());

    // Linkskanten-Resize triggert beides: wasMoved (X aendert sich) UND
    // wasResized (Breite aendert sich). Die untenstehenden separaten Branches
    // wuerden dann naheinander setTopLeftPosition() (haelt Breite fest, rechte
    // Kante wandert) und setSize() (haelt X fest) rufen — Ergebnis: die rechte
    // Kante haut nach rechts ab. Hier zusammen behandeln, rechte Kante aus den
    // (vom Constrainer bereits korrigierten) bounds halten, linke snappen,
    // Breite = right - left.
    if (wasMoved && wasResized) {
        const double sr = Project::getInstance()->getSampleRate();
        const long currentLeftSample  = (long) std::round((double) getBounds().getX() / zoom * sr);
        const long currentRightSample = (long) std::round((double)(getBounds().getX() + getBounds().getWidth()) / zoom * sr);

        long snappedLeft = currentLeftSample;
        if (Project::getInstance()->isSnapToGridEnabled()) {
            const bool fine = juce::ModifierKeys::getCurrentModifiersRealtime().isShiftDown();
            snappedLeft = Project::getInstance()->snapSampleToGrid(snappedLeft, zoom, fine);
        }
        if (snappedLeft < 0) snappedLeft = 0;
        if (snappedLeft >= currentRightSample) snappedLeft = currentLeftSample; // sanity

        // Overlap-Clamp: nur die linke Kante darf nicht in die Vorgaengerregion
        // hineinragen. Wir lassen clampOffsetForRegion entscheiden — die Laenge
        // wird hinterher entsprechend angepasst.
        if (parentTrack != nullptr) {
            const long maxLeft = currentRightSample - 1;
            long candidate = snappedLeft;
            candidate = parentTrack->clampOffsetForRegion(this, candidate, currentRightSample - candidate);
            if (candidate <= maxLeft) snappedLeft = candidate;
        }

        const long newNumSamples = currentRightSample - snappedLeft;

        const int newX     = (int) std::round((double) snappedLeft / sr * zoom);
        const int newWidth = (int) std::round((double) newNumSamples / sr * zoom);

        // Buffer-Read-Offset analog zur MidiRegion: linke Kante wandert in
        // Timeline um deltaLeft -> sourceStartSample wandert im Audio-Buffer
        // um den gleichen Betrag. Negative Werte (Region wird nach links
        // verlaengert) clampen auf 0.
        // Resize-Basis beim ersten Tick eines Gesture festfrieren — sonst
        // akkumulieren Rundungsfehler aus snappedLeft - getSampleOffset()
        // ueber viele Maus-Ticks. Reset auf -1 erfolgt in mouseUp am Resizer
        // (siehe AudioRegion::mouseUp).
        if (resizeBaseSampleOffset < 0) {
            resizeBaseSampleOffset = getSampleOffset();
            resizeBaseSourceStartSample = sourceStartSample;
        }
        sourceStartSample = juce::jmax(0L,
            resizeBaseSourceStartSample + (snappedLeft - resizeBaseSampleOffset));

        // Modell zuerst, dann setBounds — setBounds ruft componentMovedOr-
        // Resized reentrant auf; wir brauchen idempotentes Verhalten.
        // quantize=false: in Region::setSampleOffset wird sonst auf
        // bufferSize-Vielfaches quantisiert. In der Resize-Schleife wuerde
        // (snappedLeft - getSampleOffset()) deltaLeft pro Tick um den
        // Quantize-Verlust zu hoch werden, sourceStartSample drifted, der
        // Inhalt scrollt langsam nach links waehrend des Maus-Resizes.
        oldOffset = snappedLeft;
        if (snappedLeft != getSampleOffset())
            setSampleOffset(snappedLeft, true, true, false);

        if (newX != getBounds().getX() || newWidth != getWidth()) {
            setBounds(newX, getY(), newWidth, getHeight());
        }

        sendChangeMessage();
        dirty = true;
        return;
    }

    if (wasMoved) {
        long tracklen = Project::getInstance()->getTrackLength();
        double sampleRate = Project::getInstance()->getSampleRate();

        // Berechne Sample-Position basierend auf X-Position. std::round statt
        // truncation, sonst weicht's vom wasMoved&wasResized-Pfad um 1 Sample
        // ab — bei reentranten setBounds (durch den outer Branch) drifted
        // sampleOffset pro Maus-Move um genau diesen Sample, was beim Linkstrim
        // ueber sourceStartSample sichtbares Inhalts-Scrollen verursacht.
        long sampleNum = (long) std::round((double) getBounds().getX() / zoom * sampleRate);

        // Multi-Drag-Folger: Per-Region-Snap und -Clamp ueberspringen, sonst
        // raster jeder Follower auf sein eigenes Grid-Ziel und die Gruppe
        // driftet auseinander. Der Leader hat sein Snap/Clamp bereits
        // erledigt, der Dragger uebersetzt alle anderen mit dem effektiven
        // Pixel-Delta.
        const bool isMultiFollower =
            dragger != nullptr
            && dragger->isMultiDragInProgress()
            && dragger->getComponentBeingDragged() != this;

        if (!isMultiFollower) {
            // Snap auf Taktraster wenn global aktiviert (sonst freie Position).
            // Granularität: bei zoom >= 100 auf 1/4 Takt, sonst ganze Takte.
            // Mit gehaltener Shift-Taste: 1/4 des aktuellen Rasters für feinere
            // Platzierung.
            if (Project::getInstance()->isSnapToGridEnabled()) {
                const bool fine = juce::ModifierKeys::getCurrentModifiersRealtime().isShiftDown();
                sampleNum = Project::getInstance()->snapSampleToGrid(sampleNum, zoom, fine);
            }

            // Gerade Sample-Zahl sicherstellen
            if (sampleNum % 2 != 0)
                sampleNum -= 1;

            // Overlap-Clamp gegen die Nachbarregionen.
            if (parentTrack != nullptr)
                sampleNum = parentTrack->clampOffsetForRegion(this, sampleNum, getNumSamples());
        }

        oldOffset = sampleNum;

        // quantize=false analog zum Resize-Pfad — beim Drag wuerden
        // bufferSize-Quantize-Verluste sonst sample-akkurate Move-Operationen
        // (insb. mit Snap-to-Grid) gegen sich selbst verschieben.
        if (sampleNum != getSampleOffset()) {
            setSampleOffset(sampleNum, true, true, false);
        }

        // Boundary-Check
        if (getSampleOffset() >= tracklen * sampleRate) {
            long maxOffset = tracklen * sampleRate - getBuffer()->getNumSamples();
            maxOffset = snapToQuarterNote(maxOffset);
            setSampleOffset(maxOffset, false, false);
        }

        // Position visuell korrigieren falls durch Snap verändert.
        // std::round statt int-Truncation: durch die bufferSize-Quantisierung in
        // Region::setSampleOffset verliert sampleOffset einige Samples (z.B.
        // 44100 → 44032). Mit Truncation würde correctedX dann auf 19 statt 20
        // landen, und der MultiComponentDragger könnte die Region nicht mehr
        // sauber an die linke Kante zurückbewegen, weil delta=-19 vom Snap
        // wieder auf 0 gerundet wird.
        int correctedX = (int)std::round((double)getSampleOffset() / sampleRate * zoom);
        if (correctedX != getBounds().getX()) {
            setTopLeftPosition(correctedX, getY());
        }

        Logger::getCurrentLogger()->writeToLog("AudioRegion snapped to quarter note at sample " + String(sampleOffset));
    }

    if (wasResized) {
        const long offset = getSampleOffset();
        long newNumSamples = (long)((double)getWidth() / zoom * sampleRate);

        // Snap der rechten Kante auf das Taktraster, wenn global aktiv —
        // gleiches Verhalten wie beim Verschieben der Region. Mit gehaltener
        // Shift-Taste 1/4 des aktuellen Rasters für feinere Platzierung.
        if (Project::getInstance()->isSnapToGridEnabled()) {
            const bool fine = juce::ModifierKeys::getCurrentModifiersRealtime().isShiftDown();
            const long rightEdge = offset + newNumSamples;
            const long snappedRightEdge = Project::getInstance()->snapSampleToGrid(rightEdge, zoom, fine);
            const long snappedLen = snappedRightEdge - offset;
            if (snappedLen > 0)
                newNumSamples = snappedLen;
        }

        // Begrenze die rechte Kante so, dass sie nicht in die nächste Region läuft.
        if (parentTrack != nullptr) {
            const long maxLen = parentTrack->clampLengthForRegion(this, offset, newNumSamples);
            if (maxLen < newNumSamples)
                newNumSamples = maxLen;
        }

        const int newWidth = (int)((double)newNumSamples / sampleRate * zoom);
        if (newWidth != getWidth())
            setSize(newWidth, getHeight());

        sendChangeMessage();
    }

    dirty = true;
}

void AudioRegion::setClipRefId(String id) {
    this->clipRefId = id;
}

String AudioRegion::getClipRefid() {
    return clipRefId;
}

float AudioRegion::getPitch() {
    return pitch;
}


void AudioRegion::setPitch(float pitch) {

    if (pitch < 0.1) {
        pitch = 0.1;
    }
    if (pitch > 2) {
        pitch = 2;
    }

    this->pitch = pitch;
    
    this->thumbnail->reset(2, sampleRate);
    this->thumbnail->addBlock(0, *audioBuffer, 0, audioBuffer->getNumSamples());
    // updateThumb();

    /*
    if (audioBuffer != nullptr) 
    {
        if (this->tempBufferLeft == nullptr)
            this->tempBufferLeft = new float[audioBuffer->getNumSamples() * 2];
        if (this->tempBufferRight == nullptr)
            this->tempBufferRight = new float[audioBuffer->getNumSamples() * 2];
        interpolatorLeft->reset();
        interpolatorRight->reset();
        interpolatorLeft->process(this->pitch, audioBuffer->getReadPointer(0), tempBufferLeft, audioBuffer->getNumSamples()/this->pitch);
        interpolatorRight->process(this->pitch, audioBuffer->getReadPointer(1), tempBufferRight, audioBuffer->getNumSamples() / this->pitch);
        audioBufferProcessed = new AudioSampleBuffer(2,audioBuffer->getNumSamples()*2);
        audioBufferProcessed->copyFrom(0, 0, tempBufferLeft, audioBuffer->getNumSamples() * 2);
        audioBufferProcessed->copyFrom(1, 0, tempBufferRight, audioBuffer->getNumSamples() * 2);
    }
    */
}



juce::Array<juce::PropertyComponent*>& AudioRegion::getProperties()
{
    juce::Array<juce::PropertyComponent*> properties = Region::getProperties();

    properties = Array<PropertyComponent*>();

    pitchProp = new SliderPropertyComponent(*pitchValue, "Pitch", 0.1, 2.0f, 0.1, 1, true);
    properties.add(pitchProp);
    return properties;
}

void AudioRegion::createProperties()
{
    // Region::createProperties() is already called by Region::Region() during base-class
    // construction (where virtual dispatch resolves to Region::createProperties, not here).
    // Calling it again here would overwrite nameValue/nameListener raw pointers without
    // freeing the previous objects, leaking one SimpleValueSource per AudioRegion instance.
    pitchValue = new Value();
    pitchListener = new PitchListener(*pitchValue, this);
    pitchValue->setValue(pitch);
}

void AudioRegion::resizeAudio(int length)
{
    const int oldSize = audioBuffer->getNumSamples();
    AudioSampleBuffer* temp = new AudioSampleBuffer(2, oldSize + length);
    temp->copyFrom(0, 0, audioBuffer->getReadPointer(0), oldSize);
    temp->copyFrom(1, 0, audioBuffer->getReadPointer(1), oldSize);
    // Zero the appended tail — it is uninitialised heap memory until
    // setSample fills it in, and the thumbnail can read it before then.
    temp->clear(oldSize, length);

    // Publish the new buffer before deleting the old one. The thumbnail
    // timer (UI thread) reads audioBuffer concurrently with this audio-thread
    // resize; a brief delete-before-assign window let it dereference a freed
    // pointer and crash. Pointer assignment is atomic on supported platforms,
    // so the reader either sees the old or new buffer — never a dangling one.
    AudioSampleBuffer* old = audioBuffer;
    audioBuffer = temp;
    delete old;
}

double AudioRegion::getQuarterNoteLength() {
    // Berechne die L�nge einer Viertelnote in Samples
    double bpm = Project::getInstance()->getTempo();
    double quarterNoteInSeconds = 60.0 / bpm;  // Eine Viertelnote in Sekunden
    return quarterNoteInSeconds * sampleRate;  // In Samples umrechnen
}

long AudioRegion::snapToQuarterNote(long samplePosition) {
    double quarterNoteLength = getQuarterNoteLength();

    // Auf n�chste Viertelnote runden
    long quarterNoteIndex = (long)std::round(samplePosition / quarterNoteLength);
    return (long)(quarterNoteIndex * quarterNoteLength);
}

void AudioRegion::updateSnapSettings() {
    double quarterNoteLength = getQuarterNoteLength();
    double quarterNotePixels = (quarterNoteLength / sampleRate) * zoom;

    // Raster auf Viertelnoten setzen
    if (dragger != nullptr) {
        dragger->setRaster(quarterNotePixels);
    }
    if (constrainer != nullptr) {
        constrainer->setRaster(quarterNotePixels);
    }
}


