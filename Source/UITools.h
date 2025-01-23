/*
  ==============================================================================

	UITools.h
	Created: 23 Jan 2025 7:43:59pm
	Author:  mpue

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class UITools
{

public:

	/**
	* Snaps the current given location to a given grid
	*
	* @param location the location to be snapped
	* @param raster the raster size
	* @param tolerance the tolerance to cosult while snapping
	* @return
	*/

	static double snap(int location, int raster, int tolerance) {

		int toleranceWindow = (int)(raster / tolerance);

		if (location > 0) {


			if ((fmod(location, raster)) > toleranceWindow) {
				location = location + (raster - (int)(fmod(location, raster)));
			}
			else {
				location = location - (int)(fmod(location, raster));
			}
		}
		else {
			if ((fmod(location, raster)) < toleranceWindow) {
				location = location + (raster - (int)(fmod(location, raster))) - raster;
			}
			else {
				location = location - (int)(fmod(location, raster)) - raster;
			}
		}

		return location;
	}
};