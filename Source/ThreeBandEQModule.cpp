/*
  ==============================================================================

    ThreeBandEQModule.cpp
    Created: [Current Date]
    Author:  [Author Name]

    3-Band Equalizer Module implementation

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "ThreeBandEQModule.h"
#include "Connection.h"

using juce::Justification;
using juce::ImageCache;
using juce::IIRCoefficients;

//==============================================================================
ThreeBandEQModule::ThreeBandEQModule(double sampleRate, int bufferSize)
{
    this->sampleRate = sampleRate;
    this->buffersize = bufferSize;
    
    // Create IIR filters for each band
    lowShelfFilter = std::make_unique<juce::IIRFilter>();
    midPeakFilter = std::make_unique<juce::IIRFilter>();
    highShelfFilter = std::make_unique<juce::IIRFilter>();
    
    // Initialize working buffers
    tempBuffer1.setSize(1, bufferSize);
    tempBuffer2.setSize(1, bufferSize);
    
    // Initialize filter coefficients
    updateLowBandFilter();
    updateMidBandFilter();
    updateHighBandFilter();
    
    setSize(160, 240);  // Taller than other modules to accommodate more controls
    nameLabel->setJustificationType(Justification::left);
    nameLabel->setTopLeftPosition(18, 2);
    
    setName("3-Band EQ");
    
    editable = false;
    prefab = true;
}

ThreeBandEQModule::~ThreeBandEQModule()
{
}

void ThreeBandEQModule::configurePins()
{
    // Audio input and output
    Pin* audioIn = new Pin(Pin::Type::AUDIO);
    audioIn->direction = Pin::Direction::IN;
    audioIn->listeners.push_back(this);
    audioIn->setName("In");
    audioIn->setDescription("Audio input for the 3-band equalizer.");
    
    Pin* audioOut = new Pin(Pin::Type::AUDIO);
    audioOut->direction = Pin::Direction::OUT;
    audioOut->listeners.push_back(this);
    audioOut->setName("Out");
    audioOut->setDescription("Processed audio output from the 3-band equalizer.");
    
    // Low band controls
    Pin* lowFreq = new Pin(Pin::Type::VALUE);
    lowFreq->direction = Pin::Direction::IN;
    lowFreq->listeners.push_back(this);
    lowFreq->setName("LF");
    lowFreq->setDescription("Low band frequency control. Typical range [20..500] Hz.");
    
    Pin* lowQ = new Pin(Pin::Type::VALUE);
    lowQ->direction = Pin::Direction::IN;
    lowQ->listeners.push_back(this);
    lowQ->setName("LQ");
    lowQ->setDescription("Low band Q-factor control. Typical range [0.1..10].");
    
    Pin* lowGain = new Pin(Pin::Type::VALUE);
    lowGain->direction = Pin::Direction::IN;
    lowGain->listeners.push_back(this);
    lowGain->setName("LG");
    lowGain->setDescription("Low band gain control. Typical range [0.1..10] (linear) or [-20..20] dB.");
    
    // Mid band controls  
    Pin* midFreq = new Pin(Pin::Type::VALUE);
    midFreq->direction = Pin::Direction::IN;
    midFreq->listeners.push_back(this);
    midFreq->setName("MF");
    midFreq->setDescription("Mid band frequency control. Typical range [200..5000] Hz.");
    
    Pin* midQ = new Pin(Pin::Type::VALUE);
    midQ->direction = Pin::Direction::IN;
    midQ->listeners.push_back(this);
    midQ->setName("MQ");
    midQ->setDescription("Mid band Q-factor control. Typical range [0.1..10].");
    
    Pin* midGain = new Pin(Pin::Type::VALUE);
    midGain->direction = Pin::Direction::IN;
    midGain->listeners.push_back(this);
    midGain->setName("MG");
    midGain->setDescription("Mid band gain control. Typical range [0.1..10] (linear) or [-20..20] dB.");
    
    // High band controls
    Pin* highFreq = new Pin(Pin::Type::VALUE);
    highFreq->direction = Pin::Direction::IN;
    highFreq->listeners.push_back(this);
    highFreq->setName("HF");
    highFreq->setDescription("High band frequency control. Typical range [2000..20000] Hz.");
    
    Pin* highQ = new Pin(Pin::Type::VALUE);
    highQ->direction = Pin::Direction::IN;
    highQ->listeners.push_back(this);
    highQ->setName("HQ");
    highQ->setDescription("High band Q-factor control. Typical range [0.1..10].");
    
    Pin* highGain = new Pin(Pin::Type::VALUE);
    highGain->direction = Pin::Direction::IN;
    highGain->listeners.push_back(this);
    highGain->setName("HG");
    highGain->setDescription("High band gain control. Typical range [0.1..10] (linear) or [-20..20] dB.");
    
    // Add pins in order: audio in/out first, then controls by band
    addPin(Pin::Direction::IN, audioIn);     // 0 - Audio In
    addPin(Pin::Direction::OUT, audioOut);   // 1 - Audio Out
    addPin(Pin::Direction::IN, lowFreq);     // 2 - Low Frequency
    addPin(Pin::Direction::IN, lowQ);        // 3 - Low Q
    addPin(Pin::Direction::IN, lowGain);     // 4 - Low Gain
    addPin(Pin::Direction::IN, midFreq);     // 5 - Mid Frequency
    addPin(Pin::Direction::IN, midQ);        // 6 - Mid Q
    addPin(Pin::Direction::IN, midGain);     // 7 - Mid Gain
    addPin(Pin::Direction::IN, highFreq);    // 8 - High Frequency
    addPin(Pin::Direction::IN, highQ);       // 9 - High Q
    addPin(Pin::Direction::IN, highGain);    // 10 - High Gain
}

void ThreeBandEQModule::paint(juce::Graphics& g)
{
    Module::paint(g);
    
    //// Draw EQ visualization or icon
    g.setColour(juce::Colours::white);
    
    // Draw frequency response curve representation
    juce::Path eqCurve;
    const int startX = 70;
    const int startY = 50;
    const int width = 70;
    const int height = 40;
    
    // Draw frame
    g.drawRect(startX, startY, width, height, 1);
    
    // Draw approximate EQ curve
    eqCurve.startNewSubPath(startX, startY + height/2);
    eqCurve.lineTo(startX + width/4, startY + height/4);      // Low boost
    eqCurve.lineTo(startX + width/2, startY + height/3);      // Mid
    eqCurve.lineTo(startX + 3*width/4, startY + height/4);    // High boost
    eqCurve.lineTo(startX + width, startY + height/2);
    
    g.strokePath(eqCurve, juce::PathStrokeType(1.5f));
    
    // Draw band labels
    g.setFont(8.0f);
    g.drawText("L", startX + 5, startY + height + 5, 10, 10, juce::Justification::centred);
    g.drawText("M", startX + width/2 - 5, startY + height + 5, 10, 10, juce::Justification::centred);
    g.drawText("H", startX + width - 15, startY + height + 5, 10, 10, juce::Justification::centred);
    
    // Draw "EQ" label
    g.setFont(12.0f);
    g.drawText("EQ", startX + width/2 - 10, startY + height + 15, 20, 15, juce::Justification::centred);
}

void ThreeBandEQModule::process()
{
    // Check if we have audio input
    if (pins.at(0)->getConnections().size() < 1 || 
        pins.at(0)->getConnections().at(0)->getAudioBuffer() == nullptr ||
        pins.at(1)->getAudioBuffer() == nullptr)
    {
        // No input - clear output
        pins.at(1)->getAudioBuffer()->clear();
        return;
    }
    
    // Get input buffer
    const float* inputData = pins.at(0)->getConnections().at(0)->getAudioBuffer()->getReadPointer(0);
    float* outputData = pins.at(1)->getAudioBuffer()->getWritePointer(0);
    
    // Check and update parameters
    bool needsUpdate = false;
    
    // Low band parameters (pins 2, 3, 4)
    if (pins.at(2)->getConnections().size() > 0)
    {
        float newFreq = juce::jlimit(20.0f, 500.0f, pins.at(2)->getConnections().at(0)->getValue());
        if (std::abs(newFreq - lowBand.frequency) > 0.1f)
        {
            lowBand.frequency = newFreq;
            lowBand.needsUpdate = true;
        }
    }
    
    if (pins.at(3)->getConnections().size() > 0)
    {
        float newQ = juce::jlimit(0.1f, 10.0f, pins.at(3)->getConnections().at(0)->getValue());
        if (std::abs(newQ - lowBand.q) > 0.01f)
        {
            lowBand.q = newQ;
            lowBand.needsUpdate = true;
        }
    }
    
    if (pins.at(4)->getConnections().size() > 0)
    {
        float newGain = juce::jlimit(0.1f, 10.0f, pins.at(4)->getConnections().at(0)->getValue());
        if (std::abs(newGain - lowBand.gain) > 0.01f)
        {
            lowBand.gain = newGain;
            lowBand.needsUpdate = true;
        }
    }
    
    // Mid band parameters (pins 5, 6, 7)
    if (pins.at(5)->getConnections().size() > 0)
    {
        float newFreq = juce::jlimit(200.0f, 5000.0f, pins.at(5)->getConnections().at(0)->getValue());
        if (std::abs(newFreq - midBand.frequency) > 0.1f)
        {
            midBand.frequency = newFreq;
            midBand.needsUpdate = true;
        }
    }
    
    if (pins.at(6)->getConnections().size() > 0)
    {
        float newQ = juce::jlimit(0.1f, 10.0f, pins.at(6)->getConnections().at(0)->getValue());
        if (std::abs(newQ - midBand.q) > 0.01f)
        {
            midBand.q = newQ;
            midBand.needsUpdate = true;
        }
    }
    
    if (pins.at(7)->getConnections().size() > 0)
    {
        float newGain = juce::jlimit(0.1f, 10.0f, pins.at(7)->getConnections().at(0)->getValue());
        if (std::abs(newGain - midBand.gain) > 0.01f)
        {
            midBand.gain = newGain;
            midBand.needsUpdate = true;
        }
    }
    
    // High band parameters (pins 8, 9, 10)
    if (pins.at(8)->getConnections().size() > 0)
    {
        float newFreq = juce::jlimit(2000.0f, 20000.0f, pins.at(8)->getConnections().at(0)->getValue());
        if (std::abs(newFreq - highBand.frequency) > 0.1f)
        {
            highBand.frequency = newFreq;
            highBand.needsUpdate = true;
        }
    }
    
    if (pins.at(9)->getConnections().size() > 0)
    {
        float newQ = juce::jlimit(0.1f, 10.0f, pins.at(9)->getConnections().at(0)->getValue());
        if (std::abs(newQ - highBand.q) > 0.01f)
        {
            highBand.q = newQ;
            highBand.needsUpdate = true;
        }
    }
    
    if (pins.at(10)->getConnections().size() > 0)
    {
        float newGain = juce::jlimit(0.1f, 10.0f, pins.at(10)->getConnections().at(0)->getValue());
        if (std::abs(newGain - highBand.gain) > 0.01f)
        {
            highBand.gain = newGain;
            highBand.needsUpdate = true;
        }
    }
    
    // Update filter coefficients if needed
    if (lowBand.needsUpdate)
    {
        updateLowBandFilter();
        lowBand.needsUpdate = false;
    }
    
    if (midBand.needsUpdate)
    {
        updateMidBandFilter();
        midBand.needsUpdate = false;
    }
    
    if (highBand.needsUpdate)
    {
        updateHighBandFilter();
        highBand.needsUpdate = false;
    }
    
    // Copy input to first temp buffer
    tempBuffer1.copyFrom(0, 0, inputData, buffersize);
    
    // Apply low shelf filter
    lowShelfFilter->processSamples(tempBuffer1.getWritePointer(0), buffersize);
    
    // Copy to second temp buffer and apply mid peaking filter
    tempBuffer2.copyFrom(0, 0, tempBuffer1.getReadPointer(0), buffersize);
    midPeakFilter->processSamples(tempBuffer2.getWritePointer(0), buffersize);
    
    // Copy back to first temp buffer and apply high shelf filter
    tempBuffer1.copyFrom(0, 0, tempBuffer2.getReadPointer(0), buffersize);
    highShelfFilter->processSamples(tempBuffer1.getWritePointer(0), buffersize);
    
    // Copy final result to output
    for (int i = 0; i < buffersize; ++i)
    {
        outputData[i] = tempBuffer1.getSample(0, i);
    }
}

void ThreeBandEQModule::updateLowBandFilter()
{
    // Create low shelf filter coefficients
    auto coeffs = IIRCoefficients::makeLowShelf(sampleRate, lowBand.frequency, lowBand.q, lowBand.gain);
    lowShelfFilter->setCoefficients(coeffs);
}

void ThreeBandEQModule::updateMidBandFilter()
{
    // Create peaking filter coefficients for mid band
    auto coeffs = IIRCoefficients::makePeakFilter(sampleRate, midBand.frequency, midBand.q, midBand.gain);
    midPeakFilter->setCoefficients(coeffs);
}

void ThreeBandEQModule::updateHighBandFilter()
{
    // Create high shelf filter coefficients
    auto coeffs = IIRCoefficients::makeHighShelf(sampleRate, highBand.frequency, highBand.q, highBand.gain);
    highShelfFilter->setCoefficients(coeffs);
}