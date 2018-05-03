#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "AudioThumbnailComponent.h"
#include "SamplerModule.h"
#include "AudioManager.h"
#include "Project.h"

class SamplerModule;

class SamplePropertiesPanel  : public Component,
public Timer,
public Button::Listener,
public Slider::Listener
{
public:
    //==============================================================================
    SamplePropertiesPanel ();
    ~SamplePropertiesPanel();
    
    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
    void setModule(SamplerModule* module);
    void setThumbnail(AudioThumbnailComponent* thumbnail);
    void openSample();
    void updateValues();
    void startRecording();
    void stopRecording();
    void timerCallback() override;
    void updateRecordingTime();
    void updateThumb();

    //[/UserMethods]
    
    void paint (Graphics& g) override;
    void resized() override;
    void buttonClicked (Button* buttonThatWasClicked) override;
    void sliderValueChanged (Slider* sliderThatWasMoved) override;

    
private:
    //[UserVariables]   -- You can add your own custom variables in this section.
    SamplerModule* module;
    AudioThumbnailComponent* thumb;
    bool recording = false;
    int seconds = 0;
    //[/UserVariables]
    
    //==============================================================================
    ScopedPointer<TextButton> loadSampleButton;
    ScopedPointer<Slider> sampleStartSlider;
    ScopedPointer<Label> startLabel;
    ScopedPointer<Label> endLabel;
    ScopedPointer<Slider> sampleEndSlider;
    ScopedPointer<ToggleButton> loopToggleButton;
    ScopedPointer<Slider> pitchSlider;
    ScopedPointer<Label> pitchLabel;
    ScopedPointer<TextButton> recordButton;
    ScopedPointer<ImageButton> recordActiveButton;
    ScopedPointer<Label> timeLabel;
    
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SamplePropertiesPanel)
};

class SampleEditor  : public Component, public MidiKeyboardStateListener, public MidiInputCallback, public DragAndDropTarget
{
public:

    SampleEditor (int buffersize, float sampleRate, AudioFormatManager* manager, SamplerModule* module);
    ~SampleEditor();

    void paint (Graphics& g) override;
    void resized() override;

    virtual void handleNoteOff (MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity) override;
    virtual void handleNoteOn (MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity) override;
    
    bool isInterestedInDragSource (const SourceDetails& dragSourceDetails) override;
    virtual void itemDropped (const SourceDetails& dragSourceDetails) override;
    
    void handleIncomingMidiMessage (MidiInput* source, const MidiMessage& message) override;
    
    SamplePropertiesPanel* getPanel() {
        return propertiesPanel;
    }
    
private:
    MidiKeyboardState state;
    MidiKeyboardComponent::Orientation orientation = MidiKeyboardComponent::Orientation::horizontalKeyboard;

    int buffersize;

    ScopedPointer<MidiKeyboardComponent> midiKeyboard;
    AudioThumbnailComponent* component;
    ScopedPointer<SamplePropertiesPanel> propertiesPanel;
    
    SamplerModule* sampleModule;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleEditor)
};

class SamplerModule : public Module, public Timer
{
public:
    SamplerModule(double sampleRate, int buffersize, AudioFormatManager* manager);
    ~SamplerModule();
    
    void paint (Graphics& g) override;
    
    void setAmplitude(float amplitude);
    void process() override;
    
    virtual void configurePins() override;
    virtual String getCategory() override {
        return "Sound sources";
    }
    
    virtual void timerCallback() override;
    virtual void eventReceived(Event *e) override;
    void loadSample(InputStream* is, int sampler);
    void setSamplePath(String sample, int sampler);
    String getSamplePath(int i);
    void setPitch(float pitch, int samplerIndex);
    float getPitch(int samplerIndex);
    void updatePush2Display();
    
    bool hasSampleAt(int i);
    
    AudioSampleBuffer* getBuffer();
    

    
    void selectSample(int i);
    int getCurrentSamplerIndex() {
        return currentSampler;
    }
    
    void setLoop(bool loop) {
        sampler[currentSampler]->setLoop(loop);
    }
    
    bool isLoop() {
        return sampler[currentSampler]->isLoop();
    }
    
    Sampler* getCurrentSampler() {
#ifdef USE_PUSH
        if (sampler[currentSampler] != nullptr) {
            pushSamplePosX = ((float)ableton::Push2DisplayBitmap::kWidth / sampler[currentSampler]->getSampleLength())* sampler[currentSampler]->getStartPosition();
            updatePush2Display();
        }
#endif
        
        return sampler[currentSampler];
    }

#ifdef USE_PUSH
    void updatePush2Pads() {

        AudioDeviceManager* deviceManager = AudioManager::getInstance()->getDeviceManager();
        
        if (deviceManager->getDefaultMidiOutput() != nullptr) {
            deviceManager->getDefaultMidiOutput()->sendMessageNow(MidiMessage(0xb0,86,1));
            for (int i = 0; i < 128;i++) {
                deviceManager->getDefaultMidiOutput()->sendMessageNow(MidiMessage(0x90,i,0));
                if (sampler[i] != nullptr && sampler[i]->hasSample()) {
                    
                    deviceManager->getDefaultMidiOutput()->sendMessageNow(MidiMessage(0x90,i,0x7e));
                }
            }
        }

    }
#endif
    Sampler* getSamplerAt(int samplerNum) {
        return sampler[samplerNum];
    }
    
    bool isRecording() {
        return recording;
    }
    void startRecording();
    void stopRecording();
    
    SampleEditor* getEditor() {
        return editor;
    }
    
private:
    float samplePosX = 20;
    float pushSamplePosX = 0;
    AudioFormatManager* manager;
    AudioThumbnailCache* cache = nullptr;
    AudioThumbnail* thumbnail = nullptr;
    Sampler* sampler[128] = { nullptr };
    int currentSample = 0;
    int currentEnvelope = -1;
    bool gate = false;
    String samplePaths[128];
    
    float bufferLeft [1024*1024] = {0};
    float bufferRight [1024*1024] = {0};
    
    virtual void setSampleRate(float rate) override;
    virtual void setBuffersize(int buffersize) override;
    
    AudioSampleBuffer* recordingBuffer = nullptr;
    SampleEditor* editor = nullptr;
    int currentSampler = 0;
    bool recording = false;
    
    int numRecordedSamples = 0;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SamplerModule)
};
