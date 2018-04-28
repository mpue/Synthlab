#pragma once


#include "../JuceLibraryCode/JuceHeader.h"
#include "Cell.h"
#include "Pin.h"

class SequenceEditor  : public Component,
                        public MouseListener,
                        public AudioProcessorPlayer,
                        public Timer,
                        public Button::Listener
{
public:

    SequenceEditor (Pin* output);
    ~SequenceEditor();

    void handleIncomingMidiMessage (MidiInput* source, const MidiMessage& message) override;
    virtual void handlePartialSysexMessage (MidiInput* source,
                                            const uint8* messageData,
                                            int numBytesSoFar,
                                            double timestamp) override;
    void setDeviceManager(AudioDeviceManager* manager);
    uint8_t decodeSysex(uint8_t *sysex, uint8_t *data, uint8_t len);

    void sendConfig();

    void timerCallback() override;
    void paint (Graphics& g) override;
    void resized() override;
    void buttonClicked (Button* buttonThatWasClicked) override;
    void mouseMove (const MouseEvent& e) override;
    void mouseDown (const MouseEvent& e) override;
    void mouseDrag (const MouseEvent& e) override;
    void mouseUp (const MouseEvent& e) override;
    bool keyPressed (const KeyPress& key) override;
    void modifierKeysChanged (const ModifierKeys& modifiers) override;

    struct SequencePanel : public Component {
        
        friend class SequenceEditor;
        
        void paint (Graphics& g) override {
            
            g.fillAll (Colour (0xff323e44));
            
            g.setColour(juce::Colours::green);
            g.fillRect(selectedCol * cellSize, selectedRow*cellSize,cellSize, cellSize);
            
            for (int x = 0; x < gridWidth;x++) {
                for (int y = 0; y < gridHeight; y++) {
                    
                    int velocity = grid[x][y].getVelocity();
                    
                    if (currentStep == x) {
                         g.setColour(juce::Colours::orange);
                    }
                    else {
                        if (grid[x][y].isEnabled()) {
                            
                            g.setColour(juce::Colour::fromRGB(velocity*2,165,0));
                        }
                        else {
                            g.setColour(juce::Colours::darkgrey);
                        }
                    }
                    
                    g.fillRect(x*cellSize+2, y*cellSize+2,cellSize-4 ,cellSize-4);
                    
                    g.setColour(juce::Colours::white);
                    
                    int octave = grid[x][y].getNote() / 12;
                    
                    if (showVelocity) {
                        
                        g.drawText(String(velocity),x*cellSize+1, y*cellSize+1, cellSize,cellSize,Justification::centred);
                    }
                    else {
                        g.drawText(notes[grid[x][y].getNote() % 12]+String(octave),x*cellSize+1, y*cellSize+1, cellSize,cellSize,Justification::centred);
                    }
                }
            }
        }
    };

private:

    static Cell grid[32][6];

    int mouseX;
    int mouseY;

    static int selectedCol;
    static int selectedRow;

    const static int gridWidth = 32;
    const static int gridHeight = 6;
    const static int cellSize = 32;

    static const String notes[128];
    
    static int currentStep;

    static bool showVelocity;
    static bool optionPressed;

    float tempo = 120;
    
    AudioDeviceManager* deviceManager;

    Pin* output = nullptr;

    ScopedPointer<TextButton> playButton;
    ScopedPointer<TextButton> recordButton;
    Viewport* view = nullptr;
    SequencePanel* content = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SequenceEditor)
};


