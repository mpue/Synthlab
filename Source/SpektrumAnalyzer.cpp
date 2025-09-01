/*
  ==============================================================================

    SpektrumAnalyzer.cpp
    Created: 1 Sep 2025 4:43:02pm
    Author:  mpue

  ==============================================================================
*/

#include "SpektrumAnalyzer.h"

SpectrumAnalyzer::SpectrumAnalyzer()
    : forwardFFT(fftOrder),
    window(fftSize, juce::dsp::WindowingFunction<float>::hann)
{
    // Initialize buffers
    std::fill(fifo, fifo + fftSize, 0.0f);
    std::fill(fftData, fftData + 2 * fftSize, 0.0f);

    // Initialize bar data vectors
    barData.resize(numBars, 0.0f);
    smoothedBarData.resize(numBars, 0.0f);
    peakBarData.resize(numBars, 0.0f);
    peakHoldTimers.resize(numBars, 0);

    // Start timer for display updates (60 FPS for smooth animation)
    startTimerHz(60);
}

SpectrumAnalyzer::~SpectrumAnalyzer()
{
    stopTimer();
}

void SpectrumAnalyzer::paint(juce::Graphics& g)
{
    drawBackground(g);
    drawFrequencyGrid(g);
    drawSpectrumBars(g);
}

void SpectrumAnalyzer::resized()
{
    repaint();
}

void SpectrumAnalyzer::timerCallback()
{
    if (nextFFTBlockReady)
    {
        calculateNextSpectrum();
        updateBarData();
        nextFFTBlockReady = false;
        repaint();
    }
    else
    {
        // Update peak decay even when no new data
        bool needsRepaint = false;
        for (int i = 0; i < numBars; ++i)
        {
            if (peakHoldTimers[i] > 0)
            {
                peakHoldTimers[i]--;
            }
            else
            {
                float newPeak = peakBarData[i] * peakDecayRate;
                if (std::abs(newPeak - peakBarData[i]) > 0.001f)
                {
                    peakBarData[i] = newPeak;
                    needsRepaint = true;
                }
            }
        }
        if (needsRepaint) repaint();
    }
}

void SpectrumAnalyzer::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    processAudioBuffer(bufferToFill);
}

void SpectrumAnalyzer::processAudioBuffer(const juce::AudioSourceChannelInfo& bufferToFill)
{
    if (bufferToFill.buffer == nullptr) return;

    const int numChannels = bufferToFill.buffer->getNumChannels();
    const int numSamples = bufferToFill.numSamples;

    // Ensure we have a valid channel to analyze
    if (channelToAnalyze >= numChannels) return;

    const float* channelData = bufferToFill.buffer->getReadPointer(channelToAnalyze, bufferToFill.startSample);

    // Push samples into FIFO buffer
    for (int i = 0; i < numSamples; ++i)
    {
        pushSampleIntoFifo(channelData[i]);
    }
}

void SpectrumAnalyzer::pushSampleIntoFifo(float sample) noexcept
{
    if (fifoIndex == fftSize)
    {
        if (!nextFFTBlockReady)
        {
            std::fill(fftData, fftData + 2 * fftSize, 0.0f);
            std::copy(fifo, fifo + fftSize, fftData);
            nextFFTBlockReady = true;
        }
        fifoIndex = 0;
    }

    fifo[fifoIndex++] = sample;
}

void SpectrumAnalyzer::calculateNextSpectrum()
{
    // Apply windowing function
    window.multiplyWithWindowingTable(fftData, fftSize);

    // Perform FFT
    forwardFFT.performFrequencyOnlyForwardTransform(fftData);
}

void SpectrumAnalyzer::updateBarData()
{
    const float nyquistFreq = static_cast<float>(sampleRate * 0.5);

    for (int barIndex = 0; barIndex < numBars; ++barIndex)
    {
        // Map bar to frequency range (logarithmic)
        float proportion = static_cast<float>(barIndex) / static_cast<float>(numBars - 1);
        float frequency = minFrequency * std::pow(maxFrequency / minFrequency, proportion);

        // Map frequency to FFT bin
        int fftBin = static_cast<int>((frequency / nyquistFreq) * (fftSize / 2));
        fftBin = juce::jlimit(0, fftSize / 2 - 1, fftBin);

        // Get magnitude and convert to decibels
        float magnitude = fftData[fftBin];
        float db = magnitude > 0.0f ? juce::Decibels::gainToDecibels(magnitude) - 48.0f : minDecibels;

        // Normalize to 0-1 range
        float normalizedLevel = juce::jlimit(0.0f, 1.0f,
            juce::jmap(db, minDecibels, maxDecibels, 0.0f, 1.0f));

        // Apply smoothing
        smoothedBarData[barIndex] = smoothedBarData[barIndex] * smoothingFactor +
            normalizedLevel * (1.0f - smoothingFactor);

        barData[barIndex] = smoothedBarData[barIndex];

        // Update peaks
        if (barData[barIndex] > peakBarData[barIndex])
        {
            peakBarData[barIndex] = barData[barIndex];
            peakHoldTimers[barIndex] = peakHoldTime;
        }
    }
}

void SpectrumAnalyzer::drawBackground(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Create gradient background
    juce::ColourGradient backgroundGradient(
        theme.background.brighter(0.1f), bounds.getTopLeft(),
        theme.background.darker(0.2f), bounds.getBottomLeft(),
        false);

    g.setGradientFill(backgroundGradient);
    g.fillRect(bounds);

    // Add subtle vignette effect
    juce::ColourGradient vignette(
        juce::Colours::transparentBlack, bounds.getCentre(),
        juce::Colours::black.withAlpha(0.3f), bounds.getTopLeft(),
        true);

    g.setGradientFill(vignette);
    g.fillRect(bounds);
}

void SpectrumAnalyzer::drawFrequencyGrid(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour(theme.gridColour);

    // Draw frequency grid lines
    std::vector<float> frequencies = { 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000 };

    for (float freq : frequencies)
    {
        if (freq >= minFrequency && freq <= maxFrequency)
        {
            float proportion = std::log(freq / minFrequency) / std::log(maxFrequency / minFrequency);
            float x = proportion * bounds.getWidth();

            g.drawVerticalLine(static_cast<int>(x), bounds.getY(), bounds.getBottom());

            // Draw frequency labels
            g.setColour(theme.textColour);
            juce::String label = (freq >= 1000) ? juce::String(freq / 1000, 1) + "k" : juce::String(static_cast<int>(freq));

            g.drawText(label,
                juce::Rectangle<float>(x - 15, bounds.getBottom() - 20, 30, 15),
                juce::Justification::centred, true);
            g.setColour(theme.gridColour);
        }
    }

    // Draw decibel grid lines
    for (int db = static_cast<int>(minDecibels); db <= static_cast<int>(maxDecibels); db += 20)
    {
        float y = juce::jmap(static_cast<float>(db), minDecibels, maxDecibels, bounds.getBottom(), bounds.getY());
        g.drawHorizontalLine(static_cast<int>(y), bounds.getX(), bounds.getRight());

        // Draw decibel labels
        g.setColour(theme.textColour);
        g.drawText(juce::String(db) + "dB",
            juce::Rectangle<float>(5, y - 8, 35, 16),
            juce::Justification::centredLeft, true);
        g.setColour(theme.gridColour);
    }
}

void SpectrumAnalyzer::drawSpectrumBars(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    float barWidth = bounds.getWidth() / static_cast<float>(numBars);
    float maxBarHeight = bounds.getHeight() - 25; // Leave space for labels

    for (int i = 0; i < numBars; ++i)
    {
        float x = i * barWidth;
        float barHeight = barData[i] * maxBarHeight;
        float peakHeight = peakBarData[i] * maxBarHeight;

        drawBar(g, i, x, barWidth * 0.8f, barHeight, peakHeight); // 0.8f for spacing between bars
    }
}

void SpectrumAnalyzer::drawBar(juce::Graphics& g, int barIndex, float x, float width, float height, float peakHeight)
{
    auto bounds = getLocalBounds().toFloat();
    float bottom = bounds.getBottom() - 25; // Account for label space
    float normalizedHeight = height / (bounds.getHeight() - 25);

    if (height > 1.0f)
    {
        // Create gradient for the bar
        juce::Colour barColour = getBarColour(normalizedHeight);

        juce::ColourGradient barGradient(
            barColour.brighter(0.3f), x + width * 0.5f, bottom - height,
            barColour.darker(0.2f), x + width * 0.5f, bottom,
            false);

        // Add glow effect for higher frequencies
        if (normalizedHeight > 0.6f)
        {
            g.setColour(theme.glowColour);
            g.fillRoundedRectangle(x - 1, bottom - height - 2, width + 2, height + 2, 2.0f);
        }

        // Draw main bar
        g.setGradientFill(barGradient);
        g.fillRoundedRectangle(x, bottom - height, width, height, 1.0f);

        // Add highlight on top edge
        g.setColour(barColour.brighter(0.5f));
        g.fillRoundedRectangle(x, bottom - height, width, 2.0f, 1.0f);
    }

    // Draw peak indicator
    if (peakHeight > 2.0f)
    {
        g.setColour(theme.peakColour);
        g.fillRoundedRectangle(x, bottom - peakHeight - 1, width, 2.0f, 1.0f);

        // Add glow to peak
        g.setColour(theme.peakColour.withAlpha(0.5f));
        g.fillRoundedRectangle(x - 1, bottom - peakHeight - 2, width + 2, 4.0f, 2.0f);
    }
}

juce::Colour SpectrumAnalyzer::getBarColour(float normalizedHeight) const
{
    if (normalizedHeight < 0.33f)
        return theme.barBaseColour;
    else if (normalizedHeight < 0.66f)
        return theme.barMidColour;
    else
        return theme.barTopColour;
}

// Configuration methods
void SpectrumAnalyzer::setChannelToAnalyze(int channelIndex)
{
    channelToAnalyze = juce::jmax(0, channelIndex);
}

void SpectrumAnalyzer::setRefreshRate(int hz)
{
    if (hz > 0 && hz <= 120)
    {
        stopTimer();
        startTimerHz(hz);
    }
}

void SpectrumAnalyzer::setFrequencyRange(float minFreq, float maxFreq)
{
    minFrequency = juce::jmax(1.0f, minFreq);
    maxFrequency = juce::jmin(static_cast<float>(sampleRate * 0.5), maxFreq);
    repaint();
}

void SpectrumAnalyzer::setDecibelRange(float minDb, float maxDb)
{
    minDecibels = minDb;
    maxDecibels = maxDb;
    repaint();
}

void SpectrumAnalyzer::setNumberOfBars(int numBars)
{
    if (numBars > 0 && numBars <= 256)
    {
        this->numBars = numBars;
        barData.resize(numBars, 0.0f);
        smoothedBarData.resize(numBars, 0.0f);
        peakBarData.resize(numBars, 0.0f);
        peakHoldTimers.resize(numBars, 0);
        repaint();
    }
}

void SpectrumAnalyzer::setSampleRate(double newSampleRate)
{
    sampleRate = newSampleRate;
    maxFrequency = juce::jmin(maxFrequency, static_cast<float>(sampleRate * 0.5));
}