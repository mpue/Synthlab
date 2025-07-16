#include "BounceThread.h"

#include "../MainComponent.h"
#include "../PluginModule.h"

#include <JuceHeader.h>

void BounceThread::run()
{
    main->bouncing = true;
    main->running = true;

    const int sampleRate = 44100;
    const int numChannels = 2;
    const int blockSize = 128;
    const int totalSamples = static_cast<int>(durationInSeconds * sampleRate);

    juce::AudioBuffer<float> buffer(numChannels, totalSamples);
    buffer.clear();

    main->prepareToPlay(blockSize, sampleRate);
    if (main->getEditor())
        main->getEditor()->prepareToPlay(blockSize, sampleRate);

    Track* track = nullptr;
    for (auto* t : main->navigator->getTracks())
    {
        if (t && t->isEnabled())
        {
            track = t;
            break;
        }
    }

    PluginModule* pluginModule = nullptr;
    if (track)
    {
        for (auto* mod : *main->getEditor()->getModule()->getModules())
        {
            if (auto* pm = dynamic_cast<PluginModule*>(mod))
            {
                pluginModule = pm;
                pluginModule->setSampleRate(sampleRate);
                pluginModule->setBuffersize(blockSize);
                pluginModule->getPlugin()->setNonRealtime(true);
                pluginModule->setBounceMode(true);
                break;
            }
        }
    }

    if (!track || !pluginModule)
    {
        main->bouncing = false;
        return;
    }

    MidiBuffer fullMidi = buildMidiBufferForTrack(track, sampleRate);

    for (int pos = 0; pos < totalSamples && !threadShouldExit(); pos += blockSize)
    {
        const int numSamples = std::min(blockSize, totalSamples - pos);

        // MIDI für aktuellen Block extrahieren
        MidiBuffer midiSlice = getMidiSlice(fullMidi, pos, numSamples);
        pluginModule->setCurrentSample(pos);
        pluginModule->process(&midiSlice);

        // Plugin-Ausgänge holen
        auto* outL = pluginModule->getPins().at(2)->getAudioBuffer();
        auto* outR = pluginModule->getPins().at(3)->getAudioBuffer();

        if (outL && outR)
        {
            // Temporärer Buffer zum sicheren Arbeiten mit numSamples
            juce::AudioBuffer<float> blockBuffer(2, numSamples);
            blockBuffer.clear();

            blockBuffer.copyFrom(0, 0, *outL, 0, 0, numSamples);
            blockBuffer.copyFrom(1, 0, *outR, 0, 0, numSamples);

            // Optional: Lautstärke etwas runter, um Clipping zu vermeiden
            blockBuffer.applyGain(0.8f);

            // In Zielbuffer schreiben
            buffer.addFrom(0, pos, blockBuffer, 0, 0, numSamples);
            buffer.addFrom(1, pos, blockBuffer, 1, 0, numSamples);
        }

        onProgress((float)pos / totalSamples);
    }


    main->releaseResources();

    // WAV schreiben
    juce::WavAudioFormat wavFormat;
    std::unique_ptr<juce::FileOutputStream> stream(outputFile.createOutputStream());

    if (stream)
    {
        std::unique_ptr<juce::AudioFormatWriter> writer(
            wavFormat.createWriterFor(stream.get(), sampleRate, numChannels, 16, {}, 0));
        if (writer)
        {
            stream.release(); // Übergabe des Besitzes an den Writer
            writer->writeFromAudioSampleBuffer(buffer, 0, totalSamples);
        }
    }

    pluginModule->getPlugin()->setNonRealtime(false);
    pluginModule->setBounceMode(false);
    main->bouncing = false;
    onProgress(1.0f);
}

MidiBuffer BounceThread::buildMidiBufferForTrack(Track* track, double sampleRate)
{
    MidiBuffer buffer;

    for (auto* region : track->getRegions())
    {
        if (auto* midiRegion = dynamic_cast<MidiRegion*>(region))
        {
            MidiMessageSequence* sequence = midiRegion->getMidiMessageSequence();
            const int regionStartSample = midiRegion->getSampleOffset();

            for (int i = 0; i < sequence->getNumEvents(); ++i)
            {
                const auto& msg = sequence->getEventPointer(i)->message;
                
                int samplePos = static_cast<int>(msg.getTimeStamp()) + regionStartSample;
                buffer.addEvent(msg, samplePos);
            }
        }
    }

    return buffer;
}

MidiBuffer BounceThread::getMidiSlice(const MidiBuffer& source, int startSample, int numSamples)
{
    MidiBuffer slice;
    for (const auto metadata : source)
    {
        int pos = metadata.samplePosition;
        if (pos >= startSample && pos < startSample + numSamples)
            slice.addEvent(metadata.getMessage(), pos - startSample);
    }
    return slice;
}
