#pragma once

#include <juce_audio_formats/juce_audio_formats.h>
#include <stdexcept>

class SmartSample
{
public:
    SmartSample() = default;

    void loadFromFile(const juce::File& file, double engineSampleRate)
    {
        juce::AudioFormatManager formatManager;
        formatManager.registerBasicFormats(); // WAV, AIFF, FLAC, MP3

        std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));

        if (!reader)
            throw std::runtime_error("Could not create reader for file: " + file.getFullPathName().toStdString());

        originalSampleRate = reader->sampleRate;
        numChannels = static_cast<int>(reader->numChannels);
        lengthInSamples = static_cast<int>(reader->lengthInSamples);
        filePath = file.getFullPathName();

        if (lengthInSamples <= 0 || numChannels <= 0)
            throw std::runtime_error("Invalid sample format: " + file.getFileName().toStdString());

        // Temporärer Puffer zum Einlesen
        juce::AudioSampleBuffer tempBuffer(numChannels, lengthInSamples);
        if (!reader->read(&tempBuffer, 0, lengthInSamples, 0, true, true))
            throw std::runtime_error("Failed to read from file: " + file.getFullPathName().toStdString());

        // Resample wenn nötig
        if (std::abs(originalSampleRate - engineSampleRate) > 1.0)
        {
            buffer = resample(tempBuffer, originalSampleRate, engineSampleRate);
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

    juce::AudioSampleBuffer resample(const juce::AudioSampleBuffer& input,
        double fromRate, double toRate)
    {

        const int numChannels = input.getNumChannels();
        const int originalSamples = input.getNumSamples();
        const double ratio = toRate / fromRate;
        const int estimatedSamples = static_cast<int>(std::ceil(originalSamples * ratio));

        if (numChannels == 0 || originalSamples == 0)
            return juce::AudioSampleBuffer(); // gib leeren Buffer zurück

        juce::AudioSampleBuffer output(numChannels, estimatedSamples);
        output.clear();

        for (int ch = 0; ch < numChannels; ++ch)
        {
            juce::LagrangeInterpolator interpolator;
            interpolator.reset();

            const float* inputPtr = input.getReadPointer(ch);
            float* outputPtr = output.getWritePointer(ch);

            int inputIndex = 0;
            int outputIndex = 0;

            while (outputIndex < estimatedSamples && inputIndex + 4 < originalSamples) // 4-Sample Safety Margin
            {
                int remaining = estimatedSamples - outputIndex;
                int blockSize = juce::jmin(128, remaining); // blockweise verarbeiten

                int inputUsed = interpolator.process(ratio,
                    inputPtr + inputIndex,
                    outputPtr + outputIndex,
                    blockSize);

                inputIndex += inputUsed;
                outputIndex += blockSize;
            }

            // Fülle den Rest (falls noch nötig) mit 0
            if (outputIndex < estimatedSamples)
                juce::FloatVectorOperations::clear(outputPtr + outputIndex, estimatedSamples - outputIndex);
        }

        return output;
    }

};
