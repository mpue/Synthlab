#include "BounceThread.h"

#include "../MainComponent.h"
#include "../PluginModule.h"
#include "../AudioOut.h"
#include "../AuxOut.h"
#include "../AudioIn.h"
#include "../Mixer.h"
#include "../TrackIn.h"

#include <JuceHeader.h>
#include <map>

void BounceThread::run()
{

    const int sampleRate = Project::getInstance()->getSampleRate();
    const int numChannels = 2;
    const int blockSize = Project::getInstance()->getBufferSize();
    const int totalSamples = static_cast<int>(durationInSeconds * sampleRate);

    DBG("=== BOUNCE START ===");
    DBG("Sample Rate: " << sampleRate);
    DBG("Block Size: " << blockSize);
    DBG("Total Samples: " << totalSamples);
    DBG("Duration: " << durationInSeconds << " seconds");

    juce::AudioBuffer<float> buffer(numChannels, totalSamples);
    buffer.clear();

    // Alle Plugin-Module für Bounce-Modus vorbereiten
    for (auto* mod : *main->getEditor()->getModule()->getModules())
    {
        if (auto* pm = dynamic_cast<PluginModule*>(mod))
        {
            pm->setSampleRate(sampleRate);
            pm->setBuffersize(blockSize);
            pm->getPlugin()->setNonRealtime(true);
            pm->setBounceMode(true);
            DBG("Prepared plugin: " << pm->getName());
        }
        if (auto* ti = dynamic_cast<TrackIn*>(mod)) {
            ti->setBouncing(true);
        }
    }
    
    main->setRunning(false);

    // MIDI-Buffer für jeden Track einzeln aufbauen
    std::map<Track*, MidiBuffer> trackMidiBuffers;
    for (auto* track : main->navigator->getTracks())
    {
        if (track && track->isEnabled())
        {
            trackMidiBuffers[track] = buildMidiBufferForTrack(track, sampleRate);
            int midiEventCount = 0;
            for (const auto metadata : trackMidiBuffers[track])
                midiEventCount++;
            DBG("Track '" << track->getName() << "' has " << midiEventCount << " MIDI events");
        }
    }

    auto fastCos = [](double x) -> float { return static_cast<float>(std::cos(x)); };
    auto fastSin = [](double x) -> float { return static_cast<float>(std::sin(x)); };

    int blocksWithAudio = 0;
    float maxMagnitude = 0.0f;

    for (int pos = 0; pos < totalSamples && !threadShouldExit(); pos += blockSize)
    {
        const int numSamples = std::min(blockSize, totalSamples - pos);

        main->navigator->setSamplePosition(pos);

        // Module verarbeiten
        main->processModule(main->getEditor()->getModule());

        for (auto* mod : *main->getEditor()->getModule()->getModules())
        {
            if (auto* ti = dynamic_cast<TrackIn*>(mod)) {
                ti->bounce();
            }
        }

        // Plugins mit MIDI verarbeiten
        for (auto* track : main->navigator->getTracks())
        {
            if (track && track->isEnabled() && track->getPluginModule())
            {
                PluginModule* pluginModule = track->getPluginModule();
                MidiBuffer trackMidiSlice = getMidiSlice(trackMidiBuffers[track], pos, numSamples);

                int midiCount = 0;
                for (const auto metadata : trackMidiSlice)
                    midiCount++;

                pluginModule->setCurrentSample(pos);
                pluginModule->process(&trackMidiSlice);

            }
        }

        // Mix-Buffer
        juce::AudioBuffer<float> mixBuffer(numChannels, numSamples);
        mixBuffer.clear();

        auto& mixer = *main->getEditor()->getMixer();
        std::vector<AudioOut*> outputChannels = mixer.getOutputChannels();

        // **DEBUG: Prüfe AudioOut-Pins**
        for (int o = 0; o < (int)outputChannels.size(); ++o)
        {
            Mixer::Channel* meta = Mixer::getInstance()->getChannel(Mixer::Channel::Type::OUT, o);
            const float chVol = (meta && !meta->mute) ? meta->volume : 0.0f;

            auto& pins = outputChannels[o]->getPins();
            for (size_t p = 0; p < pins.size(); ++p)
            {
                if (pins[p]->getConnections().empty() || !pins[p]->getConnections()[0]) continue;
                auto* buf = pins[p]->getConnections()[0]->getAudioBuffer();
                if (!buf) continue;

                float bufMag = buf->getMagnitude(0, std::min(numSamples, buf->getNumSamples()));
                if (bufMag > 0.001f)
                    DBG("AudioOut[" << o << "] Pin[" << p << "] magnitude: " << bufMag << ", volume: " << chVol);

                const float* src = buf->getReadPointer(0);
                if (!src) continue;

                int samplesToRead = std::min(numSamples, buf->getNumSamples());

                const double pan = meta ? meta->pan : 0.0;
                const float gainL = fastCos((M_PI * (pan + 1) / 4));
                const float gainR = fastSin((M_PI * (pan + 1) / 4));
                float gain = (p == 0) ? gainL * chVol : gainR * chVol;
                int targetChannel = (p == 0) ? 0 : 1;

                if (targetChannel < numChannels)
                {
                    for (int i = 0; i < samplesToRead; ++i)
                        mixBuffer.getWritePointer(targetChannel)[i] += src[i] * gain;
                }
            }
        }

        // Aux-Kanäle (ohne Debug)
        std::vector<AuxOut*> auxChannels = mixer.getAuxChannels();
        for (int a = 0; a < (int)auxChannels.size(); ++a)
        {
            Mixer::Channel* auxMeta = Mixer::getInstance()->getChannel(Mixer::Channel::Type::AUX, a);
            if (!auxMeta || !mixer.auxChannelIsValid(a, 0)) continue;

            const float auxVol = (auxMeta && !auxMeta->mute) ? auxMeta->volume : 0.0f;
            const double auxPan = auxMeta ? auxMeta->pan : 0.0;
            const float auxGainL = fastCos((M_PI * (auxPan + 1) / 4));
            const float auxGainR = fastSin((M_PI * (auxPan + 1) / 4));

            auto& auxPins = auxChannels[a]->getPins();
            for (size_t p = 0; p < auxPins.size() && p < 2; ++p)
            {
                if (auxPins[p]->getConnections().empty() || !auxPins[p]->getConnections()[0]) continue;
                auto* auxBuf = auxPins[p]->getConnections()[0]->getAudioBuffer();
                if (!auxBuf) continue;

                const float* auxSrc = auxBuf->getReadPointer(0);
                if (!auxSrc) continue;

                int samplesToRead = std::min(numSamples, auxBuf->getNumSamples());
                float auxGain = (p == 0) ? auxGainL * auxVol : auxGainR * auxVol;
                int targetChannel = (p == 0) ? 0 : 1;

                if (targetChannel < numChannels)
                {
                    for (int i = 0; i < samplesToRead; ++i)
                        mixBuffer.getWritePointer(targetChannel)[i] += auxSrc[i] * auxGain;
                }
            }
        }

        mixBuffer.applyGain(0.8f);

        float blockMag = mixBuffer.getMagnitude(0, numSamples);
        if (blockMag > 0.001f)
        {
            blocksWithAudio++;
            maxMagnitude = std::max(maxMagnitude, blockMag);
            DBG("Block " << (pos / blockSize) << " mixBuffer magnitude: " << blockMag);
        }

        for (int ch = 0; ch < numChannels; ++ch)
        {
            buffer.addFrom(ch, pos, mixBuffer, ch, 0, numSamples);
        }

        onProgress((float)pos / totalSamples);
    }

    DBG("=== BOUNCE COMPLETE ===");
    DBG("Blocks with audio: " << blocksWithAudio);
    DBG("Max magnitude: " << maxMagnitude);
    DBG("Final buffer magnitude: " << buffer.getMagnitude(0, totalSamples));

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

    // Alle Plugin-Module wieder auf Realtime umstellen
    for (auto* mod : *main->getEditor()->getModule()->getModules())
    {
        if (auto* pm = dynamic_cast<PluginModule*>(mod))
        {
            pm->getPlugin()->setNonRealtime(false);
            pm->setBounceMode(false);
        }
        if (auto* ti = dynamic_cast<TrackIn*>(mod)) {
            ti->setBouncing(false);
        }

    }

	main->navigator->setSamplePosition(0);    
    main->setRunning(true);
    main->setBouncing(false);

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
