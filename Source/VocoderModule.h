/*
  ==============================================================================

    VocoderModule.h
    Created: [Date]
    Author:  Matthias Pueski

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"

//==============================================================================
/**
 * 32-Band Vocoder Modul
 * 
 * Ein Vocoder analysiert ein Modulatorsignal (z.B. Stimme) und wendet dessen
 * spektrale Charakteristik auf ein Trägersignal (Carrier, z.B. Synthesizer) an.
 * 
 * Das Modul verwendet 32 Bandpassfilter, um das Frequenzspektrum aufzuteilen,
 * gefolgt von Envelope-Followern für jedes Band.
 */
class VocoderModule : public Module
{
public:
    VocoderModule(double sampleRate, int buffersize);
    ~VocoderModule() override;

    void paint(juce::Graphics& g) override;
    void process() override;
    void configurePins() override;
    
    juce::String getCategory() override { return "Audio FX"; }
    juce::String getDescription() override;
    
    void setSampleRate(float rate) override;
    void setBuffersize(int buffersize) override;
    
    void createProperties() override;

private:
    static constexpr int NUM_BANDS = 32;
    
    // Bandpass-Filter Struktur für jedes Band
    struct VocoderBand
    {
        juce::IIRFilter modulatorFilter;   // Filter für Modulator-Signal
        juce::IIRFilter carrierFilter;     // Filter für Carrier-Signal
        float envelope;                     // Aktuelle Envelope
        float envelopeAttack;              // Attack-Koeffizient
        float envelopeRelease;             // Release-Koeffizient
        
        VocoderBand() : envelope(0.0f), envelopeAttack(0.01f), envelopeRelease(0.01f) {}
    };
    
    std::vector<VocoderBand> bands;
    
    // Parameter
    float formantShift;      // Verschiebung der Formanten (-2 bis +2 Oktaven)
    float attack;            // Envelope Attack Zeit in Sekunden
    float release;           // Envelope Release Zeit in Sekunden
    float mix;               // Dry/Wet Mix (0 = dry, 1 = wet)
    float bandGain;          // Verstärkung pro Band
    
    // Hilfsklassen für Properties
    struct FormantShiftListener : juce::Value::Listener
    {
        FormantShiftListener(juce::Value& v, VocoderModule* mod) : mod(mod), value(v) 
        { 
            value.addListener(this); 
        }
        ~FormantShiftListener() {}
        void valueChanged(juce::Value& value) override;
        VocoderModule* mod;
        juce::Value value;
    };
    
    struct AttackListener : juce::Value::Listener
    {
        AttackListener(juce::Value& v, VocoderModule* mod) : mod(mod), value(v) 
        { 
            value.addListener(this); 
        }
        ~AttackListener() {}
        void valueChanged(juce::Value& value) override;
        VocoderModule* mod;
        juce::Value value;
    };
    
    struct ReleaseListener : juce::Value::Listener
    {
        ReleaseListener(juce::Value& v, VocoderModule* mod) : mod(mod), value(v) 
        { 
            value.addListener(this); 
        }
        ~ReleaseListener() {}
        void valueChanged(juce::Value& value) override;
        VocoderModule* mod;
        juce::Value value;
    };
    
    struct MixListener : juce::Value::Listener
    {
        MixListener(juce::Value& v, VocoderModule* mod) : mod(mod), value(v) 
        { 
            value.addListener(this); 
        }
        ~MixListener() {}
        void valueChanged(juce::Value& value) override;
        VocoderModule* mod;
        juce::Value value;
    };
    
    // Property-Komponenten
    juce::Value* formantShiftValue;
    juce::Value* attackValue;
    juce::Value* releaseValue;
    juce::Value* mixValue;
    
    FormantShiftListener* formantShiftListener;
    AttackListener* attackListener;
    ReleaseListener* releaseListener;
    MixListener* mixListener;
    
    juce::PropertyComponent* formantShiftProp;
    juce::PropertyComponent* attackProp;
    juce::PropertyComponent* releaseProp;
    juce::PropertyComponent* mixProp;
    
    // Hilfsmethoden
    void updateFilterCoefficients();
    void updateEnvelopeCoefficients();
    float calculateBandFrequency(int bandIndex);
    
    juce::Image* image;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VocoderModule)
};
