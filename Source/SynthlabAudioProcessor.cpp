/*
  ==============================================================================

    SynthlabAudioProcessor.cpp
    Created: 18 May 2018 10:06:16am
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "SynthlabAudioProcessor.h"
#include "SynthEditor.h"
#include "Module.h"
#include "AudioEngine/Sampler.h"
#include "Mixer.h"
#include "MidiClock.h"
#include "MidiGate.h"
#include "PitchBendModule.h"
#include "MidiNote.h"
#include "Knob.h"
#include "Tracks/TrackNavigator.h"

SynthlabAudioProcessor::SynthlabAudioProcessor(float sampleRate, int bufferSize, SynthEditor* editor, TransportDisplayComponent* display, TrackNavigator* navigator)  {
    this->sampleRate = sampleRate;
    this->buffersize = bufferSize;    
	this->editor = editor;
	this->transportDisplay = display;
	this->navigator = navigator;
}

SynthlabAudioProcessor::~SynthlabAudioProcessor() {

}

void SynthlabAudioProcessor::processModule(Module* m) {

	if (m != nullptr && running) {

		m->process();

		for (int i = 0; i < m->getModules()->size(); i++) {
			if (m->getModules()->at(i) != nullptr) {
				processModule(m->getModules()->at(i));
			}
		}
	}
}

void SynthlabAudioProcessor::getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill)
{
	long startTime = 0;

	if (!bouncing) {
		lastTime = (long)juce::Time::getMillisecondCounterHiRes() - currentTime;
		currentTime = (long)juce::Time::getMillisecondCounterHiRes();
		startTime = currentTime;
	}

	const int numSamples = bufferToFill.numSamples;
	auto* writableBuffer = const_cast<juce::AudioBuffer<float>*>(bufferToFill.buffer);
	if (!writableBuffer) return;

	float** outputData = const_cast<float**>(writableBuffer->getArrayOfWritePointers());
	const int outNumChans = writableBuffer->getNumChannels();
	if (!outputData || outNumChans == 0) return;

	// --- alles nullen ---
	for (int ch = 0; ch < outNumChans; ++ch)
		if (outputData[ch])
			juce::FloatVectorOperations::clear(outputData[ch], numSamples);

	// --- Sampler nur auf 0/1 ---
	if (defaultSampler && defaultSampler->isPlaying()) {
		for (int j = 0; j < numSamples; ++j) {
			float sL = defaultSampler->getOutput(0);
			float sR = defaultSampler->getOutput(1);
			if (outNumChans > 0 && outputData[0]) outputData[0][j] += sL;
			if (outNumChans > 1 && outputData[1]) outputData[1][j] += sR;
			defaultSampler->nextSample();
		}
	}

	if (!running) return;

	if (editor->getModule())
		processModule(editor->getModule());

	auto& mixer = *editor->getMixer();
	std::vector<AudioOut*> outputChannels = mixer.getOutputChannels();
	std::vector<AudioIn*>  inputChannels = mixer.getInputChannels();

	// --- Host -> Mixer Inputs ---
	for (int k = 0; k < Mixer::getInstance()->getNumInputs(); ++k) {
		Mixer::Channel* inCh = Mixer::getInstance()->getChannel(Mixer::Channel::Type::IN, k);
		if (!inCh || k >= (int)inputChannels.size()) continue;

		if (!(k < outNumChans && (k + 1) < outNumChans)) continue;

		double pan = inCh->pan;
		float gainL = fastCos((M_PI * (pan + 1) / 4));
		float gainR = fastSin((M_PI * (pan + 1) / 4));

		auto* bufL = inputChannels.at(k)->pins.at(0)->getAudioBuffer();
		auto* bufR = inputChannels.at(k)->pins.at(1)->getAudioBuffer();

		bufL->copyFrom(0, bufferToFill.startSample, *bufferToFill.buffer, k, bufferToFill.startSample, numSamples);
		bufR->copyFrom(0, bufferToFill.startSample, *bufferToFill.buffer, k + 1, bufferToFill.startSample, numSamples);
		bufL->applyGain(0, bufferToFill.startSample, numSamples, inCh->volume * gainL);
		bufR->applyGain(0, bufferToFill.startSample, numSamples, inCh->volume * gainR);

		inCh->magnitudeLeft = bufL->getMagnitude(0, bufferToFill.startSample, numSamples);
		inCh->magnitudeRight = bufR->getMagnitude(0, bufferToFill.startSample, numSamples);
	}

	// --- 1:1 Output Mapping mit Pan ---
	for (int o = 0; o < (int)outputChannels.size(); ++o) {
		Mixer::Channel* meta = Mixer::getInstance()->getChannel(Mixer::Channel::Type::OUT, o);
		const float chVol = (meta && !meta->mute) ? meta->volume : 0.0f;
		const double pan = meta ? meta->pan : 0.0;
		const float gainL = fastCos((M_PI * (pan + 1) / 4));
		const float gainR = fastSin((M_PI * (pan + 1) / 4));

		auto& pins = outputChannels[o]->getPins();
		for (size_t p = 0; p < pins.size(); ++p) {
			int hwIndex = o * pins.size() + (int)p; // Annahme: linear hintereinander
			if (hwIndex >= outNumChans) continue;

			if (pins[p]->getConnections().empty() || !pins[p]->getConnections()[0]) continue;
			auto* buf = pins[p]->getConnections()[0]->getAudioBuffer();
			if (!buf) continue;

			const float* src = buf->getReadPointer(0);
			if (!src || !outputData[hwIndex]) continue;

			float gain = (p == 0) ? gainL * chVol : gainR * chVol;
			for (int i = 0; i < numSamples; ++i)
				outputData[hwIndex][i] += src[i] * gain;
		}

		// Pegelberechnung
		if (meta && pins.size() >= 2) {
			if (pins[0]->getConnections().size() > 0)
				meta->magnitudeLeft = chVol * gainL * pins[0]->getConnections()[0]->getAudioBuffer()->getMagnitude(0, 0, numSamples);
			if (pins[1]->getConnections().size() > 0)
				meta->magnitudeRight = chVol * gainR * pins[1]->getConnections()[0]->getAudioBuffer()->getMagnitude(0, 0, numSamples);

		}
	}

	// --- Metronom auf 0/1 ---
	if (metronome && transportDisplay && transportDisplay->isMetronomeActive() && navigator->isRecording()) {
		if (metronomeBuffer.getNumSamples() != bufferToFill.numSamples)
			metronomeBuffer.setSize(2, bufferToFill.numSamples, false, false, true);

		metronome->generateBlock(metronomeBuffer, (long)navigator->getSamplePosition());

		if (outNumChans > 0) {
			auto* destL = bufferToFill.buffer->getWritePointer(0, bufferToFill.startSample);
			auto* srcL = metronomeBuffer.getReadPointer(0);
			for (int i = 0; i < numSamples; ++i) destL[i] += srcL[i];
		}
		if (outNumChans > 1) {
			auto* destR = bufferToFill.buffer->getWritePointer(1, bufferToFill.startSample);
			auto* srcR = metronomeBuffer.getReadPointer(metronomeBuffer.getNumChannels() > 1 ? 1 : 0);
			for (int i = 0; i < numSamples; ++i) destR[i] += srcR[i];
		}
	}

	// Apply master channel (volume, mute, pan) to final output
	{
		Mixer::Channel* master = Mixer::getInstance()->getMasterChannel();
		if (master) {
			const float masterVol = master->mute ? 0.0f : (float)master->volume;
			const double masterPan = master->pan;
			const float gainL = fastCos((M_PI * (masterPan + 1) / 4));
			const float gainR = fastSin((M_PI * (masterPan + 1) / 4));

			for (int ch = 0; ch < outNumChans; ++ch) {
				if (!outputData[ch]) continue;
				const float gain = (ch % 2 == 0) ? gainL * masterVol : gainR * masterVol;
				for (int i = 0; i < numSamples; ++i)
					outputData[ch][i] *= gain;
			}

			float magL = 0.0f, magR = 0.0f;
			if (outNumChans > 0 && outputData[0])
				for (int i = 0; i < numSamples; ++i) magL = std::max(magL, std::abs(outputData[0][i]));
			if (outNumChans > 1 && outputData[1])
				for (int i = 0; i < numSamples; ++i) magR = std::max(magR, std::abs(outputData[1][i]));
			master->magnitudeLeft  = magL;
			master->magnitudeRight = magR;
		}
	}

	if (!bouncing) {
		long duration = (long)juce::Time::getMillisecondCounterHiRes() - startTime;
		cpuLoad = ((float)duration / (float)lastTime) * 100.0f;
	}

	currentSample = (currentSample + 1) % numSamples;
}

void SynthlabAudioProcessor::releaseResources()
{




}

void SynthlabAudioProcessor::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
	Logger::writeToLog("prepare to play with sample rate " + String(sampleRate) + " kHz and buffer size of " + String(buffersize) + " bytes.");

	this->sampleRate = sampleRate;
	this->buffersize = samplesPerBlockExpected;

	metronome = std::make_unique<Metronome>(sampleRate, samplesPerBlockExpected);
	metronome->setBPM(120.0f); // oder Wert aus TransportDisplay
	
	defaultSampler = new Sampler(sampleRate, buffersize);

}

int SynthlabAudioProcessor::getNumActiveChannels(int i) {
	i = i - ((i >> 1) & 0x55555555);
	i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
	return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}

