#include <iomanip>
#include "SequenceEditor.h"
#include "AudioManager.h"

using juce::String;
using juce::TextButton;
using juce::Slider;
using juce::Viewport;
using juce::Graphics;
using juce::Button;
using juce::MouseEvent;
using juce::KeyPress;
using juce::ModifierKeys;
using juce::MidiMessage;
using juce::MidiInput;
using juce::Logger;
using juce::uint8;

const String SequenceEditor::notes[128] = {"C","C#", "D", "D#", "E", "F", "F#", "G", "G#" , "A" ,"A#", "B"};
int SequenceEditor::selectedCol = 0;
int SequenceEditor::selectedRow = 0;
bool SequenceEditor::optionPressed = false;
bool SequenceEditor::showVelocity = false;
Cell SequenceEditor::grid[32][6];
int SequenceEditor::currentStep = 0;

int SequenceEditor::gridWidth = 32;
int SequenceEditor::gridHeight = 6;

SequenceEditor::SequenceEditor (Pin* output)
{
    this->output = output;

    thread = new TimeSliceThread("SequenceEditor Timer");
    thread->startThread(); // Start the thread
    
    addAndMakeVisible (playButton = new TextButton ("Play"));
    playButton->setButtonText (TRANS("Play"));
    playButton->addListener (this);

    playButton->setBounds (10, 10, 150, 24);

    addAndMakeVisible (recordButton = new TextButton ("Record"));
    recordButton->setButtonText (TRANS("Record"));
    recordButton->addListener (this);

    recordButton->setBounds (170, 10, 150, 24);

    addAndMakeVisible (clearButton = new TextButton ("Clear"));
    clearButton->setButtonText (TRANS("Clear"));
    clearButton->addListener (this);
    
    clearButton->setBounds (330, 10, 150, 24);
    
    addAndMakeVisible (lengthSlider = new Slider ("lengthSlider"));
    lengthSlider->setRange (1, 64, 1);
    lengthSlider->setSliderStyle (Slider::LinearHorizontal);
    lengthSlider->setTextBoxStyle (Slider::TextBoxLeft, false, 80, 20);
    lengthSlider->addListener (this);
    lengthSlider->setBounds (500, 10, 150, 24);

    addAndMakeVisible (gateSlider = new Slider ("gateSlider"));
    gateSlider->setRange (5, 100, 1);
    gateSlider->setSliderStyle (Slider::LinearHorizontal);
    gateSlider->setTextBoxStyle (Slider::TextBoxLeft, false, 80, 20);
    gateSlider->setTextValueSuffix(" %");
    gateSlider->setValue (100, juce::dontSendNotification);
    gateSlider->addListener (this);
    gateSlider->setBounds (660, 10, 150, 24);
    
    setSize (800, 768);

    content = new SequencePanel();

    view = new Viewport();
    view->setViewedComponent(content);

    setSize(gridWidth*cellSize,gridHeight * cellSize + 100);
    setBounds(0,0,gridWidth*cellSize,gridHeight * cellSize + 100);

    for (int x = 0; x < gridWidth;x++) {
        for (int y = 0; y < gridHeight; y++) {
            Cell c = Cell();
            c.setEnabled(false);
            c.setNote(36);
            c.setVelocity(64);
            grid[x][y] = c;
        }
    }

    for (int y = 0; y < 6; y++) {
        activeNotes[y] = -1;
        noteOffTick[y] = -1;
    }

    selectedCol = 0;
    selectedRow = 0;
    addAndMakeVisible(view);

    setWantsKeyboardFocus(true);
    setInterceptsMouseClicks(true,true);
    addMouseListener(content, true);
    
}

SequenceEditor::~SequenceEditor()
{
    playButton = nullptr;
    recordButton = nullptr;
    clearButton = nullptr;
    lengthSlider = nullptr;
    gateSlider = nullptr;
    delete view;
    
    if (thread != nullptr) {
        thread->removeAllClients();
        thread->stopThread(1000); // Wait up to 1 second for thread to stop
        delete thread;
    }
}

//==============================================================================
void SequenceEditor::paint (Graphics& g)
{

}

void SequenceEditor::resized()
{
    if (content != nullptr && view != nullptr) {
        view->setTopLeftPosition(0,50);
        view->setSize(getWidth(), cellSize * 7);
        content->setSize(cellSize*32, cellSize * 6);
    }
}

void SequenceEditor::buttonClicked (Button* buttonThatWasClicked)
{

    if (buttonThatWasClicked == playButton)
    {
        if (!running) {
            if (syncMode == SyncMode::INTERNAL) {
                thread->addTimeSliceClient(this);
            }
            running = true;
            currentStep = -1; // Will be incremented to 0 on first step
            clockEventNum = 0;
        }
        else {
            if (syncMode == SyncMode::INTERNAL) {
                thread->removeTimeSliceClient(this);
            }
            running = false;
        }
    }
    else if (buttonThatWasClicked == recordButton)
    {
        if (syncMode == SyncMode::INTERNAL) {
            thread->removeTimeSliceClient(this);
        }
        else {
            running = false;
        }
    }
    else if(buttonThatWasClicked == clearButton) {
        clearSequence();
    }
}

void SequenceEditor::sliderValueChanged(juce::Slider *slider) {
    if (slider == lengthSlider) {
        int newWidth = (int)slider->getValue();
        if (newWidth >= 1 && newWidth <= 32) { // Ensure valid range
            gridWidth = newWidth;
            // Reset selected column if it's out of bounds
            if (selectedCol >= gridWidth) {
                selectedCol = gridWidth - 1;
            }
            resized(); // Update the view size
            repaint();
            if (content != nullptr) {
                content->repaint();
            }
        }
    }
    else if (slider == gateSlider) {
        gateTime = juce::jlimit (0.05f, 1.0f, (float)slider->getValue() / 100.0f);
    }
}

void SequenceEditor::setGateTime(int percent) {
    gateTime = juce::jlimit (0.05f, 1.0f, percent / 100.0f);
    if (gateSlider != nullptr) {
        gateSlider->setValue (juce::jlimit (5, 100, percent), juce::dontSendNotification);
    }
}

void SequenceEditor::mouseMove (const MouseEvent& e)
{
    mouseX = e.getPosition().getX();
    mouseY = e.getPosition().getY();

    std::function<void(void)> changeLambda =
    [=]() {  content->repaint(); };
    juce::MessageManager::callAsync(changeLambda);


}

void SequenceEditor::mouseDown (const MouseEvent& e)
{
    // Calculate grid position with bounds checking
    int gridX = e.getPosition().getX() / cellSize;
    int gridY = e.getPosition().getY() / cellSize;
    
    // Ensure we don't go out of bounds
    if (gridX >= 0 && gridX < gridWidth && gridY >= 0 && gridY < gridHeight) {
        // toggle cell
        grid[gridX][gridY].setEnabled(!grid[gridX][gridY].isEnabled());
        selectedCol = gridX;
        selectedRow = gridY;
        
        std::function<void(void)> changeLambda =
        [=]() {  content->repaint(); };
        juce::MessageManager::callAsync(changeLambda);
    }
}

void SequenceEditor::mouseDrag (const MouseEvent& e)
{
    int delta = - e.getDistanceFromDragStartY() / 20;
    
    // Calculate grid position with bounds checking
    int gridX = e.getMouseDownX() / cellSize;
    int gridY = e.getMouseDownY() / cellSize;
    
    // Ensure we don't go out of bounds
    if (gridX < 0 || gridX >= gridWidth || gridY < 0 || gridY >= gridHeight) {
        return;
    }

    if (!showVelocity) {
        int note = grid[gridX][gridY].getNote();
        if (note + delta > 127) {
            note = 127;
        }
        else if(note + delta < 0) {
            note = 0;
        }
        else
        {
            note += delta;
        }

        grid[gridX][gridY].setNote(note);
    }
    else {
        int velocity = grid[gridX][gridY].getVelocity();
        if (velocity + delta > 127) {
            velocity = 127;
        }
        else if(velocity + delta < 0) {
            velocity = 0;
        }
        else
        {
            velocity += delta;
        }

        grid[gridX][gridY].setVelocity(velocity);
    }

    sendConfig();

    std::function<void(void)> changeLambda =
    [=]() {  content->repaint(); };
    juce::MessageManager::callAsync(changeLambda);
}

void SequenceEditor::mouseUp (const MouseEvent& e)
{
}

bool SequenceEditor::keyPressed (const KeyPress& key)
{
    if (key.getKeyCode() == KeyPress::spaceKey) {
        if (!running) {
            if (syncMode == SyncMode::INTERNAL) {
                thread->addTimeSliceClient(this);
            }
            running = true;
            currentStep = -1; // Will be incremented to 0 on first step
            clockEventNum = 0;
        }
        else {
            if (syncMode == SyncMode::INTERNAL) {
                thread->removeTimeSliceClient(this);
            }
            running = false;
        }
    }
    if (key.getKeyCode() == KeyPress::returnKey) {
        grid[selectedCol][selectedRow].setEnabled(!grid[selectedCol][selectedRow].isEnabled());
    }

    int velocity = grid[selectedCol][selectedRow].getVelocity();
    int note = grid[selectedCol][selectedRow].getNote();

    if (key.getKeyCode() == KeyPress::upKey) {
        if (showVelocity) {

            if (velocity < 127) {
                velocity++;
            }
            grid[selectedCol][selectedRow].setVelocity(velocity);
        }
        else if(optionPressed) {
            if (note < 127) {
                note = note + 1;
            }
            grid[selectedCol][selectedRow].setNote(note);
        }
        else {
            if (selectedRow > 0) {
                selectedRow--;
            }
        }
    }
    if (key.getKeyCode() == KeyPress::downKey) {

        if (showVelocity) {

            if (velocity > 0) {
                velocity--;
            }
            grid[selectedCol][selectedRow].setVelocity(velocity);
        }
        else if(optionPressed) {

            if (note > 0) {
                note = note - 1;
            }

            grid[selectedCol][selectedRow].setNote(note);
        }
        else {
            if(selectedRow < gridHeight - 1) {
                selectedRow++;
            }
        }
    }
    if (key.getKeyCode() == KeyPress::leftKey) {
        if (selectedCol > 0) {
            selectedCol--;
        }
    }
    if (key.getKeyCode() == KeyPress::rightKey) {
        if(selectedCol < gridWidth - 1) {
            selectedCol++;
        }
    }

    sendConfig();
    repaint();
    content->repaint();

    return true;

}

void SequenceEditor::modifierKeysChanged (const ModifierKeys& modifiers)
{
    optionPressed = modifiers.isAltDown();
    showVelocity = modifiers.isShiftDown();
    repaint();
}

void SequenceEditor::setDeviceManager(juce::AudioDeviceManager *manager) {
    this->deviceManager = manager;
}

void SequenceEditor::clearSequence() {
    for (int x = 0; x < gridWidth;x++) {
        for (int y = 0; y < gridHeight;y++) {
            grid[x][y].setLength(0);
            grid[x][y].setEnabled(false);
            grid[x][y].setNote(36 );
            grid[x][y].setVelocity(64);
            
        }
    }
    repaint();
}

void SequenceEditor::sendConfig() {
}

uint8_t SequenceEditor::decodeSysex(uint8_t *sysex, uint8_t *data, uint8_t len) {
    uint8_t cnt;
    uint8_t cnt2 = 0;
    uint8_t bits = 0;
    for (cnt = 0; cnt < len; cnt++) {
        if ((cnt % 8) == 0) {
            bits = sysex[cnt];
        } else {
            data[cnt2++] = sysex[cnt] | ((bits & 1) << 7);
            bits >>= 1;
        }
    }
    return cnt2;
}

void SequenceEditor::handleIncomingMidiMessage (MidiInput* source, const MidiMessage& message) {

    /*
    if(message.isSysEx()) {

        Logger::writeToLog("received message with size "+ String(message.getSysExDataSize()));

        uint8_t* sysex = (uint8_t*)message.getSysExData();

        File file = File("/Users/mpue/Desktop/test.syx");

        file.create();

        FileOutputStream* fos = new FileOutputStream(file);
        fos->write(message.getRawData(), message.getRawDataSize());
        fos->flush();
        delete fos;

        uint8_t* data = new uint8_t[2048];

        // int len = decodeSysex(sysex,data+3,message.getSysExDataSize()-3);

        /// Logger::writeToLog("unpacked message with size "+ String(len));

        int col = 1;

        for (int i = 0; i < 2048 ;i++) {
            std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)data[i] << " ";
            if (col % 8 == 0) {
                std::cout << std::endl;
            }
            col++;
        }

        File file_p = File("/Users/mpue/Desktop/test.mid");

        file_p.create();

        fos = new FileOutputStream(file_p);
        fos->write(data,2048);
        fos->flush();
        delete fos;

    }
     */
    if (message.isNoteOn() || message.isNoteOff()) {
        Logger::writeToLog("received message "+message.getDescription());
        if (message.isNoteOn()) {
            grid[selectedCol][selectedRow].setNote(message.getNoteNumber());
            grid[selectedCol][selectedRow].setVelocity(message.getVelocity());
            grid[selectedCol][selectedRow].setEnabled(true);
            std::function<void(void)> changeLambda =
            [this]() { repaint(); };
            juce::MessageManager::callAsync(changeLambda);
        }
    }
}

void SequenceEditor::handlePartialSysexMessage (MidiInput* source,
                                                const uint8* messageData,
                                                int numBytesSoFar,
                                                double timestamp) {
    Logger::writeToLog("received sysex message");
}


int SequenceEditor::useTimeSlice() {
    
    CriticalSection& c = AudioManager::getInstance()->getDeviceManager()->getAudioCallbackLock();
    
    Time t;
    
    int start = t.getMilliseconds();
    
    c.enter();
    triggerNextStep();
    c.exit();

    int stop = t.getMilliseconds();
    
    int delta = stop - start;
    
    return ((60.0f / tempo) * 1000.0f - delta) / 16;
}

void SequenceEditor::setRunning(bool running) {
    this->running = running;
    if (running) {
        this->currentStep = -1; // Will be incremented to 0 on first step
        this->clockEventNum = 0;
    }
    else {
        // Flush any notes that are still gated
        for (int y = 0; y < gridHeight; y++) {
            if (activeNotes[y] >= 0) {
                Event* off = new Event("gate off", Event::Type::GATE);
                off->setMessage(MidiMessage::noteOff(1, activeNotes[y]));
                off->setValue(0);
                off->setNote(activeNotes[y]);

                if (output != nullptr) {
                    for (int i = 0; i < output->getConnections().size(); i++) {
                        output->getConnections().at(i)->sendEvent(new Event(off));
                    }
                }
                delete off;
                activeNotes[y] = -1;
                noteOffTick[y] = -1;
            }
        }
    }
    std::function<void(void)> changeLambda =
    [=]() {  content->repaint(); };
    juce::MessageManager::callAsync(changeLambda);
}

void SequenceEditor::reset() {


}

void SequenceEditor::triggerNextStep() {

    if (!running){
        return;
    }

    std::function<void(void)> async =
    [=]() {
        // First: turn off any note whose gate has elapsed
        for (int y = 0; y < gridHeight; y++) {
            if (activeNotes[y] >= 0 && noteOffTick[y] >= 0 && clockEventNum >= noteOffTick[y]) {
                Event* off = new Event("gate off", Event::Type::GATE);
                off->setMessage(MidiMessage::noteOff(1, activeNotes[y]));
                off->setValue(0);
                off->setNote(activeNotes[y]);

                if (output != nullptr) {
                    for (int i = 0; i < output->getConnections().size(); i++) {
                        output->getConnections().at(i)->sendEvent(new Event(off));
                    }
                }
                delete off;
                activeNotes[y] = -1;
                noteOffTick[y] = -1;
            }
        }

        // On step boundary, advance and trigger the new step
        if (clockEventNum % 6 == 0) {
            currentStep = (currentStep + 1) % gridWidth;
            int gateTicks = juce::jmax (1, juce::roundToInt (gateTime * 6.0f));

            for (int y = 0; y < gridHeight; y++) {

                // Safety: if a note is still active on this row (e.g. gate=100% legato), retrigger cleanly
                if (activeNotes[y] >= 0) {
                    Event* off = new Event("gate off", Event::Type::GATE);
                    off->setMessage(MidiMessage::noteOff(1, activeNotes[y]));
                    off->setValue(0);
                    off->setNote(activeNotes[y]);

                    if (output != nullptr) {
                        for (int i = 0; i < output->getConnections().size(); i++) {
                            output->getConnections().at(i)->sendEvent(new Event(off));
                        }
                    }
                    delete off;
                    activeNotes[y] = -1;
                    noteOffTick[y] = -1;
                }

                if (grid[currentStep][y].isEnabled()) {
                    int note = grid[currentStep][y].getNote();
                    int velocity = grid[currentStep][y].getVelocity();

                    Event* e = new Event("gate on", Event::Type::GATE);
                    e->setValue(velocity);
                    e->setNote(note);
                    e->setMessage(MidiMessage::noteOn(1, note, (float)velocity / 127.0f));

                    if (output != nullptr) {
                        for (int i = 0; i < output->getConnections().size(); i++) {
                            output->getConnections().at(i)->sendEvent(new Event(e));
                        }
                    }
                    delete e;

                    activeNotes[y] = note;
                    noteOffTick[y] = clockEventNum + gateTicks;
                }
            }
        }
        clockEventNum++;
    };
    juce::MessageManager::callAsync(async);

    std::function<void(void)> changeLambda =
    [=]() {  content->repaint(); };
    juce::MessageManager::callAsync(changeLambda);

}

uint8* SequenceEditor::getConfiguration() {
    
    uint8* data = new uint8[gridWidth*gridHeight*3+2];
    
    // store the dimensions of the sequence in the first two bytes
    
    data[0] = gridWidth;
    data[1] = gridHeight;
    
    int index = 2;
    
    // now serialize the grid each note is a triple containing length, note and velocity
    // if the note id not enabled we assume that the length is zero.
    
    for (int x = 0; x < gridWidth;x++) {
        for (int y = 0; y < gridHeight;y++) {
            
            if (grid[x][y].isEnabled())
                data[index] = 1;
            else
                data[index] = 0;
            
            data[index+1] = grid[x][y].getNote();
            data[index+2] = grid[x][y].getVelocity();
            index += 3;
            
        }
    }

    return data;
}

void SequenceEditor::setConfiguration(uint8* data) {
    
    // sequence size is stored in the first two bytes
    
    uint8 gridWidth = data[0];
    uint8 gridHeight = data[1];
    
    int index = 2;
    
    for (int x = 0; x < gridWidth;x++) {
        for (int y = 0; y < gridHeight;y++) {
            grid[x][y].setEnabled(data[index] > 0);
            grid[x][y].setLength(data[index]);
            grid[x][y].setNote(data[index+1]);
            grid[x][y].setVelocity(data[index+2]);
            index += 3;
            
        }
    }
    
    this->gridWidth = gridWidth;
    this->gridHeight = gridHeight;
    lengthSlider->setValue(gridWidth,juce::NotificationType::dontSendNotification);
    repaint();
}

int SequenceEditor::getConfigLength() {
    return gridWidth*gridHeight*3+2;
}
