/*
  ==============================================================================

    Logger.h
    Created: 7 Apr 2018 1:17:24pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#pragma once
#include "../JuceLibraryCode/JuceHeader.h"
namespace MidiTools {
    class Logger {
    public:
        virtual ~Logger() {};
        virtual void log(juce::String message) = 0;

    };
}

