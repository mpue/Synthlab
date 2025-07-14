/*
  ==============================================================================

	BounceThread.cpp
	Created: 14 Jul 2025 12:39:54pm
	Author:  mpue

  ==============================================================================
*/

#include "BounceThread.h"

#include <JuceHeader.h>
#include "../MainComponent.h"


void BounceThread::run() 
{
	main->bouncing = true;
	main->running = true;

	const int sampleRate = 44100;
	const int numChannels = 2;
	const int totalSamples = (int)(durationInSeconds * sampleRate);

	juce::AudioBuffer<float> buffer(numChannels, totalSamples);
	buffer.clear();

	main->prepareToPlay(main->buffersize, sampleRate);
	if (main->getEditor())
		main->getEditor()->prepareToPlay(main->buffersize, sampleRate);

	if (main->navigator)
	{
		main->navigator->ResetMarkerPosition();
		main->navigator->setSamplePosition(0);
		main->navigator->setPlaying(true);
	}

	for (int pos = 0; pos < totalSamples && !threadShouldExit(); pos += main->buffersize)
	{
		const int numSamples = std::min(main->buffersize, totalSamples - pos);
		juce::AudioBuffer<float> blockBuffer(numChannels, numSamples);
		blockBuffer.clear();

		juce::AudioSourceChannelInfo block(&blockBuffer, 0, numSamples);

		if (main->navigator)
			main->navigator->setSamplePosition(pos);

		if (main->midiProcessor)
			main->midiProcessor->processBlock(numSamples, main->navigator);

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
		std::unique_ptr<juce::AudioFormatWriter> writer(wavFormat.createWriterFor(stream.get(), sampleRate, numChannels, 16, {}, 0));
		if (writer)
		{
			stream.release();
			writer->writeFromAudioSampleBuffer(buffer, 0, totalSamples);
		}
	}

	main->bouncing = false;
}

