/*
  ==============================================================================

    FadeRegionAction.cpp
    Created: 17 Oct 2025 5:58:07pm
    Author:  mpue

  ==============================================================================
*/

#include "FadeRegionAction.h"
#include "../Tracks/AudioRegion.h"
#include "../Tracks/TrackNavigator.h"
#include "../AudioUtils.h"
#include "../Project/Project.h"
#include "../FadeDialog.h"


FadeRegionAction::FadeRegionAction(TrackNavigator* navigator) : navigator(navigator)
{
}

FadeRegionAction::~FadeRegionAction()
{
}

bool FadeRegionAction::perform()
{
	if (navigator->getCurrentTrack() == nullptr) {
		AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon, "Error", "No track selected.");
		return false;
	}

	Region* region = navigator->getCurrentTrack()->getCurrentRegion();
	if (region == nullptr) {
		AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon, "Error", "No region selected.");
		return false;
	}

	// Check if it's an audio region
	AudioRegion* audioRegion = dynamic_cast<AudioRegion*>(region);
	if (audioRegion == nullptr) {
		AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon, "Error", "Selected region is not an audio region.");
		return false;
	}

	if (audioRegion->getBuffer() == nullptr) {
		AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon, "Error", "No audio data found in the selected region.");
		return false;
	}

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

		// Show fade dialog
		auto* fadeDialog = new FadeDialog();

		fadeDialog->onApply = [this, fadeDialog, audioRegion]() {
			// Get parameters
			int fadeInMs = fadeDialog->getFadeInMs();
			int fadeOutMs = fadeDialog->getFadeOutMs();
			bool exponential = fadeDialog->isExponential();

			// Convert milliseconds to samples
			double sampleRate = Project::getInstance()->getSampleRate();
			int fadeInSamples = static_cast<int>((fadeInMs / 1000.0) * sampleRate);
			int fadeOutSamples = static_cast<int>((fadeOutMs / 1000.0) * sampleRate);

			// Apply fades
			if (exponential) {
				if (fadeInSamples > 0)
					AudioUtils::applyExponentialFadeIn(*audioRegion->getBuffer(), fadeInSamples);
				if (fadeOutSamples > 0)
					AudioUtils::applyExponentialFadeOut(*audioRegion->getBuffer(), fadeOutSamples);
			}
			else {
				if (fadeInSamples > 0)
					AudioUtils::applyFadeIn(*audioRegion->getBuffer(), fadeInSamples);
				if (fadeOutSamples > 0)
					AudioUtils::applyFadeOut(*audioRegion->getBuffer(), fadeOutSamples);
			}

			// Update thumbnail
			audioRegion->updateThumb();

			// Close dialog
			if (auto* dw = fadeDialog->findParentComponentOfClass<juce::DialogWindow>()) {
				dw->exitModalState(0);
			}
			};

		fadeDialog->onCancel = [fadeDialog]() {
			if (auto* dw = fadeDialog->findParentComponentOfClass<juce::DialogWindow>()) {
				dw->exitModalState(0);
			}
			};

		juce::DialogWindow::LaunchOptions options;
		options.content.setOwned(fadeDialog);
		options.dialogTitle = "Fade In/Out";
		options.dialogBackgroundColour = juce::Colour(0xff222222);
		options.escapeKeyTriggersCloseButton = true;
		options.useNativeTitleBar = true;
		options.resizable = false;
		options.componentToCentreAround = navigator->getTopLevelComponent();

		auto* window = options.launchAsync();
		Project::getInstance()->getOpenWindows().push_back(window);


		audioRegion->updateThumb();

		return true;
	}



    return true;
}

bool FadeRegionAction::undo()
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
