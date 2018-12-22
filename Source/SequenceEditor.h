#pragma once


#include "../JuceLibraryCode/JuceHeader.h"
#include "Cell.h"
#include "Pin.h"

class SequenceEditor  : public juce::Component,
                        public juce::AudioProcessorPlayer,
                        public juce::TimeSliceClient,
                        public juce::Button::Listener,
                        public juce::Slider::Listener
{
public:

    enum SyncMode {
        INTERNAL,
        MIDI_CLOCK
    };
    
    SequenceEditor (Pin* output);
    ~SequenceEditor();

    void handleIncomingMidiMessage (juce::MidiInput* source, const juce::MidiMessage& message) override;
    virtual void handlePartialSysexMessage (juce::MidiInput* source,
                                            const juce::uint8* messageData,
                                            int numBytesSoFar,
                                            double timestamp) override;
    void setDeviceManager(juce::AudioDeviceManager* manager);
    uint8_t decodeSysex(uint8_t *sysex, uint8_t *data, uint8_t len);

    void sendConfig();
    int getConfigLength();
    int useTimeSlice () override;
    void paint (juce::Graphics& g) override;
    void resized() override;
    void buttonClicked (juce::Button* buttonThatWasClicked) override;
    void mouseMove (const juce::MouseEvent& e) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;
    bool keyPressed (const juce::KeyPress& key) override;
    void modifierKeysChanged (const juce::ModifierKeys& modifiers) override;
    void clearSequence();
    void sliderValueChanged (juce::Slider* slider) override;
    juce::uint8* getConfiguration();
    void setConfiguration(juce::uint8* data);
    
    void setRunning(bool running);
    
    void triggerNextStep();
    
    void reset();
    
    void setTempo(float tempo) {
        this->tempo = tempo;
    }
    
    
    bool isRunning() {
        return running;
    }
    
    struct SequencePanel : public juce::Component {
        
        friend class SequenceEditor;
        
        void paint (juce::Graphics& g) override {
            
            g.fillAll (juce::Colour (0xff323e44));
            
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
                        
                        g.drawText(juce::String(velocity),x*cellSize+1, y*cellSize+1, cellSize,cellSize,juce::Justification::centred);
                    }
                    else {
                        g.drawText(notes[grid[x][y].getNote() % 12]+juce::String(octave),x*cellSize+1, y*cellSize+1, cellSize,cellSize,juce::Justification::centred);
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

    static int gridWidth;
    static int gridHeight;
    const static int cellSize = 32;

    static const juce::String notes[128];
    
    static int currentStep;

    static bool showVelocity;
    static bool optionPressed;

    float tempo = 120;
    
    juce::AudioDeviceManager* deviceManager;

    Pin* output = nullptr;

    juce::ScopedPointer<juce::TextButton> playButton;
    juce::ScopedPointer<juce::TextButton> recordButton;
    juce::ScopedPointer<juce::TextButton> clearButton;
    juce::ScopedPointer<juce::Slider> lengthSlider;
    
    juce::Viewport* view = nullptr;
    SequencePanel* content = nullptr;

    SyncMode syncMode = MIDI_CLOCK;
    int clockEventNum = 0;
    
    bool running = false;
    
    TimeSliceThread* thread;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SequenceEditor)
};


