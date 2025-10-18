/*
  ==============================================================================

    NormalizeRegionActiion.cpp
    Created: 17 Oct 2025 5:57:42pm
    Author:  mpue

  ==============================================================================
*/

#include "NormalizeRegionAction.h"
#include "../Tracks/AudioRegion.h"
#include "../Tracks/TrackNavigator.h"
#include "../AudioUtils.h"

NormalizeRegionAction::NormalizeRegionAction(TrackNavigator* navigator) : navigator(navigator)
{
}

NormalizeRegionAction::~NormalizeRegionAction()
{
}

bool NormalizeRegionAction::perform()
{
	if (navigator->getCurrentTrack() != nullptr) {
		Region* region = navigator->getCurrentTrack()->getCurrentRegion();

		// check if it's an audio region
		AudioRegion* audioRegion = dynamic_cast<AudioRegion*>(region);

		if (audioRegion != nullptr) {
			// Speichere den Original-Buffer für Undo
			targetRegion = audioRegion;
			auto* buffer = audioRegion->getBuffer();
			
			if (buffer != nullptr) {
				// Kopiere den Original-Buffer
				originalBuffer = std::make_unique<juce::AudioSampleBuffer>(
					buffer->getNumChannels(), 
					buffer->getNumSamples()
				);
				
				for (int channel = 0; channel < buffer->getNumChannels(); ++channel) {
					originalBuffer->copyFrom(channel, 0, *buffer, channel, 0, buffer->getNumSamples());
				}
				
				// Führe die Normalisierung durch
				AudioUtils::normalizeBuffer(*buffer);
				audioRegion->updateThumb();
				Project::getInstance()->getCommandManager()->commandStatusChanged();	

				return true;
			}
		}
	}

	return false;
}

bool NormalizeRegionAction::undo()
{
	if (targetRegion != nullptr && originalBuffer != nullptr) {
		auto* buffer = targetRegion->getBuffer();
		
		if (buffer != nullptr) {
			// Stelle den Original-Buffer wieder her
			for (int channel = 0; channel < originalBuffer->getNumChannels(); ++channel) {
				buffer->copyFrom(channel, 0, *originalBuffer, channel, 0, originalBuffer->getNumSamples());
			}
			
			targetRegion->updateThumb();
			return true;
		}
	}
	
	return false;
}
