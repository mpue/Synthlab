/*
  ==============================================================================

    SpektrumAnalyzer.h
    Created: 1 Sep 2025 4:43:02pm
    Author:  mpue

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class SpectrumAnalyzer : public juce::Component,
    public juce::Timer
{
public:
    SpectrumAnalyzer();
    ~SpectrumAnalyzer() override;

    // Component interface
    void paint(juce::Graphics& g) override;
    void resized() override;

    // Timer interface
    void timerCallback() override;

    // Audio processing - Main interface
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill);

    // Configuration
    void setChannelToAnalyze(int channelIndex);
    void setRefreshRate(int hz);
    void setFrequencyRange(float minFreq, float maxFreq);
    void setDecibelRange(float minDb, float maxDb);
    void setNumberOfBars(int numBars);
    void setSampleRate(double newSampleRate);

private:
    enum
    {
        fftOrder = 11,              // 2048 samples
        fftSize = 1 << fftOrder,    // 2048
        defaultNumBars = 64         // Default number of frequency bars
    };

    // FFT processing
    juce::dsp::FFT forwardFFT;
    juce::dsp::WindowingFunction<float> window;

    // Audio data buffers
    float fifo[fftSize];                    // Circular buffer for incoming samples
    float fftData[2 * fftSize];            // FFT input/output buffer
    std::vector<float> barData;            // Bar heights for display
    std::vector<float> smoothedBarData;    // Smoothed bar data
    std::vector<float> peakBarData;        // Peak hold values
    std::vector<int> peakHoldTimers;       // Peak hold timers

    int fifoIndex = 0;
    bool nextFFTBlockReady = false;
    int channelToAnalyze = 0;
    int numBars = defaultNumBars;

    // Display parameters
    float minFrequency = 20.0f;
    float maxFrequency = 20000.0f;
    float minDecibels = -80.0f;
    float maxDecibels = 0.0f;
    double sampleRate = 44100.0;

    // Smoothing and animation
    float smoothingFactor = 0.7f;
    float peakDecayRate = 0.95f;
    int peakHoldTime = 30; // frames

    // Visual styling - Amber/Bernstein theme
    struct AmberTheme
    {
        juce::Colour background = juce::Colour(0x1a, 0x0f, 0x0a);        // Dark amber background
        juce::Colour barBaseColour = juce::Colour(0xff, 0x8c, 0x00);     // Bright amber
        juce::Colour barMidColour = juce::Colour(0xff, 0xa5, 0x00);      // Medium amber
        juce::Colour barTopColour = juce::Colour(0xff, 0xd7, 0x00);      // Light amber/gold
        juce::Colour peakColour = juce::Colour(0xff, 0xff, 0xff);        // White peaks
        juce::Colour gridColour = juce::Colour(0x40, 0x2a, 0x1a);        // Dark amber grid
        juce::Colour textColour = juce::Colour(0xcc, 0x88, 0x44);        // Amber text
        juce::Colour glowColour = juce::Colour(0xff, 0x8c, 0x00).withAlpha(0.3f); // Glow effect
    } theme;

    // Helper methods
    void processAudioBuffer(const juce::AudioSourceChannelInfo& bufferToFill);
    void pushSampleIntoFifo(float sample) noexcept;
    void calculateNextSpectrum();
    void updateBarData();

    // Drawing methods
    void drawBackground(juce::Graphics& g);
    void drawFrequencyGrid(juce::Graphics& g);
    void drawSpectrumBars(juce::Graphics& g);
    void drawBar(juce::Graphics& g, int barIndex, float x, float width, float height, float peakHeight);

    // Utility methods
    juce::Colour getBarColour(float normalizedHeight) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumAnalyzer)
};