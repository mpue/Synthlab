#pragma once

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <stdexcept>
#include "../AudioManager.h"

class SmartSample
{
public:
    SmartSample() = default;

    void loadFromFile(const juce::File& file, double engineSampleRate)
    {
        std::unique_ptr<juce::AudioFormatReader> reader(
            AudioManager::getInstance()->getFormatManager()->createReaderFor(file));

        if (!reader)
            throw std::runtime_error("Could not create reader for file: " + file.getFullPathName().toStdString());

        originalSampleRate = reader->sampleRate;
        numChannels = static_cast<int>(reader->numChannels);
        lengthInSamples = static_cast<int>(reader->lengthInSamples);
        filePath = file.getFullPathName();

        if (lengthInSamples <= 0 || numChannels <= 0)
            throw std::runtime_error("Invalid sample format: " + file.getFileName().toStdString());

        // Lesepuffer vorbereiten
        juce::AudioSampleBuffer tempBuffer(numChannels, lengthInSamples);
        if (!reader->read(&tempBuffer, 0, lengthInSamples, 0, true, true))
            throw std::runtime_error("Failed to read from file: " + file.getFullPathName().toStdString());

        // Resample falls n�tig
        if (std::abs(originalSampleRate - engineSampleRate) > 1.0)
        {
            buffer = resampleWithResamplingSource(tempBuffer, originalSampleRate, engineSampleRate);
            wasResampled = true;
        }
        else
        {
            buffer.makeCopyOf(tempBuffer);
            wasResampled = false;
        }

        this->engineSampleRate = engineSampleRate;
    }

    juce::AudioSampleBuffer& getBuffer() { return buffer; }
    const juce::AudioSampleBuffer& getBuffer() const { return buffer; }

    double getOriginalSampleRate() const { return originalSampleRate; }
    double getEngineSampleRate() const { return engineSampleRate; }
    bool isResampled() const { return wasResampled; }

    int getNumChannels() const { return numChannels; }
    int getLengthInSamples() const { return lengthInSamples; }

    juce::String getFilePath() const { return filePath; }

private:
    juce::AudioSampleBuffer buffer;
    double originalSampleRate = 0.0;
    double engineSampleRate = 0.0;
    bool wasResampled = false;
    int numChannels = 0;
    int lengthInSamples = 0;
    juce::String filePath;

    juce::AudioSampleBuffer resampleWithResamplingSource(const juce::AudioSampleBuffer& input,
        double fromRate, double toRate)
    {
        const double ratio = toRate / fromRate;
        const int targetLength = static_cast<int>(std::ceil(input.getNumSamples() * ratio));

        juce::AudioSampleBuffer resultBuffer(input.getNumChannels(), targetLength);
        resultBuffer.clear();

        auto memorySource = std::make_unique<MemoryAudioSource>(input, fromRate);
        juce::ResamplingAudioSource resampler(memorySource.get(), false);
        resampler.setResamplingRatio(fromRate / toRate);
        resampler.prepareToPlay(8192, toRate); // Gr��erer Block f�r Effizienz

        // In Bl�cken verarbeiten statt alles auf einmal
        const int blockSize = 8192;
        int samplesWritten = 0;

        while (samplesWritten < targetLength)
        {
            const int numThisTime = juce::jmin(blockSize, targetLength - samplesWritten);
            juce::AudioSourceChannelInfo info(&resultBuffer, samplesWritten, numThisTime);
            resampler.getNextAudioBlock(info);
            samplesWritten += numThisTime;
        }

        resampler.releaseResources();

        return resultBuffer;
    }


    class MemoryAudioSource : public juce::AudioSource
    {
    public:
        MemoryAudioSource(const juce::AudioBuffer<float>& bufferToUse, double sampleRate)
            : buffer(bufferToUse), rate(sampleRate)
        {
        }

        void prepareToPlay(int, double) override
        {
            position = 0;
        }

        void releaseResources() override {}

        void getNextAudioBlock(const juce::AudioSourceChannelInfo& info) override
        {
            const int numSamples = info.numSamples;
            const int numAvailable = buffer.getNumSamples() - position;
            const int numToCopy = juce::jmin(numSamples, numAvailable);

            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                if (info.buffer->getNumChannels() > ch)
                    info.buffer->copyFrom(ch, info.startSample, buffer, ch, position, numToCopy);
            }

            if (numToCopy < numSamples)
            {
                for (int ch = 0; ch < info.buffer->getNumChannels(); ++ch)
                    info.buffer->clear(ch, info.startSample + numToCopy, numSamples - numToCopy);
            }

            position += numToCopy;
        }

    private:
        const juce::AudioBuffer<float>& buffer;
        int position = 0;
        double rate;
    };

};
