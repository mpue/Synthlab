#include "BounceThread.h"

#include "../MainComponent.h"
#include "../PluginModule.h"
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

    if (main->navigator)
    {
        main->navigator->ResetMarkerPosition();
        main->navigator->setSamplePosition(0);
        main->navigator->setPlaying(true);
    }

    // Nur eine aktive Spur mit Plugin wird verwendet (später ggf. erweitern)
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
       // find the plugin module which has the same channel as the track
        for (int i = 0;i < main->getEditor()->getModule()->getModules()->size();i++)        
        {
            if (dynamic_cast<PluginModule*>(main->getEditor()->getModule()->getModules()->at(i)))
            {
                pluginModule = dynamic_cast<PluginModule*>(main->getEditor()->getModule()->getModules()->at(i));
             // TODO : Find a better way to get the plugin module for the track
				pluginModule->setSampleRate(sampleRate);
				pluginModule->setBuffersize(blockSize);
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
        juce::AudioBuffer<float> blockBuffer(numChannels, numSamples);
        blockBuffer.clear();

        juce::AudioSourceChannelInfo block(&blockBuffer, 0, numSamples);
        
        MidiBuffer midiSlice = getMidiSlice(fullMidi, pos, numSamples);

        if (main->navigator)
            main->navigator->setSamplePosition(pos);

        pluginModule->process(&midiSlice);

        main->getNextAudioBlock(block);

        for (int ch = 0; ch < numChannels; ++ch)
            buffer.addFrom(ch, pos, blockBuffer, ch, 0, numSamples);

        onProgress((float)pos / totalSamples);
    }

    if (main->navigator)
        main->navigator->setPlaying(false);

    main->releaseResources();

    juce::WavAudioFormat wavFormat;
    std::unique_ptr<juce::FileOutputStream> stream(outputFile.createOutputStream());

    if (stream)
    {
        std::unique_ptr<juce::AudioFormatWriter> writer(
            wavFormat.createWriterFor(stream.get(), sampleRate, numChannels, 16, {}, 0));
        if (writer)
        {
            stream.release();
            writer->writeFromAudioSampleBuffer(buffer, 0, totalSamples);
        }
    }

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
            const int regionStartSample = midiRegion->getStartSample();

            for (int i = 0; i < sequence->getNumEvents(); ++i)
            {
                const auto& msg = sequence->getEventPointer(i)->message;
                int samplePos = static_cast<int>(msg.getTimeStamp() * sampleRate) + regionStartSample;
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
