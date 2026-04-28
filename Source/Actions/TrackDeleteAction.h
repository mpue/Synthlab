#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "../Tracks/TrackNavigator.h"
#include "../SynthEditor.h"
#include "../Pin.h"
#include "../Connection.h"

class PluginModule;

class TrackDeleteAction : public juce::UndoableAction
{
public:
    TrackDeleteAction(TrackNavigator* navigator, SynthEditor* editor);
    ~TrackDeleteAction() override;

    bool perform() override;
    bool undo() override;

private:
    struct PinLink {
        Pin* outer;
        Pin* inner;
    };

    TrackNavigator* navigator;
    SynthEditor* editor;

    Track* removedTrack = nullptr;
    int savedIndex = -1;
    PluginModule* removedModule = nullptr;

    std::vector<Connection*> removedConnections;
    std::vector<PinLink> removedPinLinks;

    bool trackInNavigator = true;
};
