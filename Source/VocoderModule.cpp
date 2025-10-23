/*
  ==============================================================================

    VocoderModule.cpp
    Created: [Date]
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "VocoderModule.h"
#include "Connection.h"

using juce::Justification;
using juce::ImageCache;

//==============================================================================
VocoderModule::VocoderModule(double sampleRate, int buffersize)
    : formantShift(1.0f),
      attack(0.01f),
      release(0.05f),
      mix(1.0f),
      bandGain(1.0f),
      formantShiftValue(nullptr),
      attackValue(nullptr),
      releaseValue(nullptr),
      mixValue(nullptr),
      formantShiftListener(nullptr),
      attackListener(nullptr),
      releaseListener(nullptr),
      mixListener(nullptr),
      formantShiftProp(nullptr),
      attackProp(nullptr),
      releaseProp(nullptr),
      mixProp(nullptr),
      image(nullptr)
{
    this->sampleRate = sampleRate;
    this->buffersize = buffersize;
    
    setSize(120, 140);
    setName("Vocoder");
    
    editable = false;
    prefab = true;
    
    // Initialisiere die 32 Bänder
    bands.resize(NUM_BANDS);
    
    // Erstelle Filter-Koeffizienten für alle Bänder
    updateFilterCoefficients();
    updateEnvelopeCoefficients();
}

VocoderModule::~VocoderModule()
{
    if (formantShiftListener != nullptr)
        delete formantShiftListener;
    if (attackListener != nullptr)
        delete attackListener;
    if (releaseListener != nullptr)
        delete releaseListener;
    if (mixListener != nullptr)
        delete mixListener;
    
    if (formantShiftValue != nullptr)
        delete formantShiftValue;
    if (attackValue != nullptr)
        delete attackValue;
    if (releaseValue != nullptr)
        delete releaseValue;
    if (mixValue != nullptr)
        delete mixValue;
}

void VocoderModule::configurePins()
{
    Pin* p1 = new Pin(Pin::Type::AUDIO);
    p1->direction = Pin::Direction::IN;
    p1->listeners.push_back(this);
    p1->setName("Mod");
    p1->setDescription("Modulator input (typically voice or speech)");
    
    Pin* p2 = new Pin(Pin::Type::AUDIO);
    p2->direction = Pin::Direction::IN;
    p2->listeners.push_back(this);
    p2->setName("Car");
    p2->setDescription("Carrier input (typically synthesizer)");
    
    Pin* p3 = new Pin(Pin::Type::AUDIO);
    p3->direction = Pin::Direction::OUT;
    p3->listeners.push_back(this);
    p3->setName("Out");
    p3->setDescription("Vocoded audio output");
    
    Pin* p4 = new Pin(Pin::Type::VALUE);
    p4->direction = Pin::Direction::IN;
    p4->listeners.push_back(this);
    p4->setName("Shft");
    p4->setDescription("Formant shift control (0.25 to 4.0, 1.0 = no shift)");
    
    Pin* p5 = new Pin(Pin::Type::VALUE);
    p5->direction = Pin::Direction::IN;
    p5->listeners.push_back(this);
    p5->setName("Mix");
    p5->setDescription("Dry/Wet mix control (0.0 = dry, 1.0 = wet)");
    
    addPin(Pin::Direction::IN, p1);   // Pin 0: Modulator
    addPin(Pin::Direction::IN, p2);   // Pin 1: Carrier
    addPin(Pin::Direction::OUT, p3);  // Pin 2: Output
    addPin(Pin::Direction::IN, p4);   // Pin 3: Formant Shift
    addPin(Pin::Direction::IN, p5);   // Pin 4: Mix
}

void VocoderModule::paint(juce::Graphics& g)
{
    Module::paint(g);
    
    // Zeichne eine visuelle Repräsentation des Vocoders
    g.setColour(juce::Colours::white);
    g.setFont(12.0f);
    g.drawText("VOCODER", 10, 45, 100, 20, Justification::centred);
    
    // Zeichne 32 vertikale Balken für die Bänder
    float barWidth = 80.0f / NUM_BANDS;
    float barX = 20.0f;
    float barY = 70.0f;
    float maxBarHeight = 50.0f;
    
    for (int i = 0; i < NUM_BANDS; i++)
    {
        float height = bands[i].envelope * maxBarHeight;
        g.setColour(juce::Colours::cyan.withAlpha(0.6f));
        g.fillRect(barX + i * barWidth, barY + maxBarHeight - height, barWidth - 1, height);
    }
    
    // Rahmen um die Visualisierung
    g.setColour(juce::Colours::white.withAlpha(0.3f));
    g.drawRect(barX - 2, barY, 80.0f + 4, maxBarHeight + 2, 1.0f);
}

void VocoderModule::process()
{
    // Hole die Audio-Buffer
    const float* modulatorIn = nullptr;
    const float* carrierIn = nullptr;
    float* out = nullptr;
    
    // Prüfe ob Modulator verbunden ist
    if (pins.at(0)->getConnections().size() >= 1 && 
        pins.at(0)->getConnections().at(0)->getAudioBuffer() != nullptr)
    {
        modulatorIn = pins.at(0)->getConnections().at(0)->getAudioBuffer()->getReadPointer(0);
    }
    
    // Prüfe ob Carrier verbunden ist
    if (pins.at(1)->getConnections().size() >= 1 && 
        pins.at(1)->getConnections().at(0)->getAudioBuffer() != nullptr)
    {
        carrierIn = pins.at(1)->getConnections().at(0)->getAudioBuffer()->getReadPointer(0);
    }
    
    // Hole Output-Buffer
    if (pins.at(2)->getAudioBuffer() != nullptr)
    {
        out = pins.at(2)->getAudioBuffer()->getWritePointer(0);
    }
    
    // Verarbeite Formant Shift Parameter
    if (pins.at(3)->getConnections().size() == 1)
    {
        float newShift = pins.at(3)->getConnections().at(0)->getValue();
        if (newShift != formantShift)
        {
            formantShift = juce::jlimit(0.25f, 4.0f, newShift);
            updateFilterCoefficients();
        }
    }
    
    // Verarbeite Mix Parameter
    if (pins.at(4)->getConnections().size() == 1)
    {
        mix = juce::jlimit(0.0f, 1.0f, pins.at(4)->getConnections().at(0)->getValue());
    }
    
    // Wenn kein Output-Buffer vorhanden, abbrechen
    if (out == nullptr)
        return;
    
    // Wenn keine Eingänge verbunden sind, Output auf 0 setzen
    if (modulatorIn == nullptr || carrierIn == nullptr)
    {
        juce::FloatVectorOperations::clear(out, buffersize);
        return;
    }
    
    // Hauptverarbeitungsschleife
    for (int sample = 0; sample < buffersize; sample++)
    {
        float outputSample = 0.0f;
        
        // Verarbeite jedes Band
        for (int bandIdx = 0; bandIdx < NUM_BANDS; bandIdx++)
        {
            VocoderBand& band = bands[bandIdx];
            
            // Filtere Modulator-Signal
            float modFiltered = band.modulatorFilter.processSingleSampleRaw(modulatorIn[sample]);
            
            // Berechne Envelope (Gleichrichtung + Glättung)
            float rectified = std::abs(modFiltered);
            
            if (rectified > band.envelope)
            {
                // Attack-Phase
                band.envelope += (rectified - band.envelope) * band.envelopeAttack;
            }
            else
            {
                // Release-Phase
                band.envelope += (rectified - band.envelope) * band.envelopeRelease;
            }
            
            // Filtere Carrier-Signal
            float carrierFiltered = band.carrierFilter.processSingleSampleRaw(carrierIn[sample]);
            
            // Moduliere Carrier mit Envelope
            outputSample += carrierFiltered * band.envelope * bandGain;
        }
        
        // Normalisiere Output (verhindere Clipping)
        outputSample = juce::jlimit(-1.0f, 1.0f, outputSample / std::sqrt((float)NUM_BANDS));
        
        // Wende Mix-Parameter an (Dry/Wet)
        out[sample] = carrierIn[sample] * (1.0f - mix) + outputSample * mix;
    }
}

void VocoderModule::updateFilterCoefficients()
{
    // Frequenzbereich: 80 Hz bis 12000 Hz, logarithmisch verteilt
    const float minFreq = 80.0f;
    const float maxFreq = 12000.0f;
    const float Q = 5.0f;  // Q-Faktor für schmale Bänder
    
    for (int i = 0; i < NUM_BANDS; i++)
    {
        // Berechne Mittenfrequenz für dieses Band (logarithmisch)
        float centerFreq = calculateBandFrequency(i);
        
        // Modulator-Filter (verwendet die Originalfrequenz)
        auto modCoeffs = juce::IIRCoefficients::makeBandPass(sampleRate, centerFreq, Q);
        bands[i].modulatorFilter.setCoefficients(modCoeffs);
        
        // Carrier-Filter (mit Formant-Shift)
        float shiftedFreq = centerFreq * formantShift;
        shiftedFreq = juce::jlimit(minFreq, maxFreq, shiftedFreq);
        auto carrierCoeffs = juce::IIRCoefficients::makeBandPass(sampleRate, shiftedFreq, Q);
        bands[i].carrierFilter.setCoefficients(carrierCoeffs);
    }
}

void VocoderModule::updateEnvelopeCoefficients()
{
    // Konvertiere Zeit in Koeffizienten (vereinfachtes Exponential-Modell)
    for (int i = 0; i < NUM_BANDS; i++)
    {
        // Attack: schnellere Anstiegszeit für höhere Frequenzen
        float attackTime = attack * (1.0f - (float)i / NUM_BANDS * 0.5f);
        bands[i].envelopeAttack = 1.0f - std::exp(-1.0f / (attackTime * sampleRate));
        
        // Release: langsamere Abfallzeit für niedrigere Frequenzen
        float releaseTime = release * (1.0f + (float)i / NUM_BANDS * 0.5f);
        bands[i].envelopeRelease = 1.0f - std::exp(-1.0f / (releaseTime * sampleRate));
    }
}

float VocoderModule::calculateBandFrequency(int bandIndex)
{
    const float minFreq = 80.0f;
    const float maxFreq = 12000.0f;
    
    // Logarithmische Verteilung der Frequenzen
    float ratio = (float)bandIndex / (float)(NUM_BANDS - 1);
    return minFreq * std::pow(maxFreq / minFreq, ratio);
}

void VocoderModule::setSampleRate(float rate)
{
    Module::setSampleRate(rate);
    updateFilterCoefficients();
    updateEnvelopeCoefficients();
}

void VocoderModule::setBuffersize(int buffersize)
{
    Module::setBuffersize(buffersize);
}

juce::String VocoderModule::getDescription()
{
    return "32-Band Vocoder\n\n"
           "A vocoder analyzes the spectral content of a modulator signal (typically voice) "
           "and applies it to a carrier signal (typically a synthesizer).\n\n"
           "Inputs:\n"
           "- Mod: Modulator input (voice/speech)\n"
           "- Car: Carrier input (synthesizer)\n"
           "- Shft: Formant shift (0.25 to 4.0)\n"
           "- Mix: Dry/Wet balance (0.0 to 1.0)\n\n"
           "Output:\n"
           "- Out: Vocoded audio\n\n"
           "Parameters:\n"
           "- Formant Shift: Shifts the frequency bands\n"
           "- Attack: Envelope follower attack time\n"
           "- Release: Envelope follower release time\n"
           "- Mix: Blend between dry carrier and vocoded signal";
}

void VocoderModule::createProperties()
{
    Module::createProperties();
    
    // Formant Shift Property
    formantShiftValue = new juce::Value(formantShift);
    formantShiftProp = new juce::SliderPropertyComponent(*formantShiftValue, "Formant Shift", 
                                                          0.25, 4.0, 0.01);
    formantShiftProp->setTooltip("Shifts the formant frequencies (1.0 = no shift)");
    formantShiftListener = new FormantShiftListener(*formantShiftValue, this);
    properties.add(formantShiftProp);
    
    // Attack Property
    attackValue = new juce::Value(attack);
    attackProp = new juce::SliderPropertyComponent(*attackValue, "Attack", 
                                                    0.001, 0.1, 0.001);
    attackProp->setTooltip("Envelope follower attack time in seconds");
    attackListener = new AttackListener(*attackValue, this);
    properties.add(attackProp);
    
    // Release Property
    releaseValue = new juce::Value(release);
    releaseProp = new juce::SliderPropertyComponent(*releaseValue, "Release", 
                                                     0.001, 0.5, 0.001);
    releaseProp->setTooltip("Envelope follower release time in seconds");
    releaseListener = new ReleaseListener(*releaseValue, this);
    properties.add(releaseProp);
    
    // Mix Property
    mixValue = new juce::Value(mix);
    mixProp = new juce::SliderPropertyComponent(*mixValue, "Mix", 
                                                 0.0, 1.0, 0.01);
    mixProp->setTooltip("Dry/Wet mix (0 = dry carrier, 1 = full vocoder)");
    mixListener = new MixListener(*mixValue, this);
    properties.add(mixProp);
}

// Listener Implementations
void VocoderModule::FormantShiftListener::valueChanged(juce::Value& value)
{
    mod->formantShift = value.getValue();
    mod->updateFilterCoefficients();
}

void VocoderModule::AttackListener::valueChanged(juce::Value& value)
{
    mod->attack = value.getValue();
    mod->updateEnvelopeCoefficients();
}

void VocoderModule::ReleaseListener::valueChanged(juce::Value& value)
{
    mod->release = value.getValue();
    mod->updateEnvelopeCoefficients();
}

void VocoderModule::MixListener::valueChanged(juce::Value& value)
{
    mod->mix = value.getValue();
}
