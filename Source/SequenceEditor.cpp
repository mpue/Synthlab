/*
  ==============================================================================

  This is an automatically generated GUI class created by the Projucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Projucer version: 5.2.1

  ------------------------------------------------------------------------------

  The Projucer is part of the JUCE library.
  Copyright (c) 2017 - ROLI Ltd.

  ==============================================================================
*/



//[Headers] You can add your own extra header files here...

#include <iomanip>
//[/Headers]

#include "SequenceEditor.h"

const String SequenceEditor::notes[128] = {"C","C#", "D", "D#", "E", "F", "F#", "G", "G#" , "A" ,"A#", "B"};
int SequenceEditor::selectedCol = 0;
int SequenceEditor::selectedRow = 0;
bool SequenceEditor::optionPressed = false;
bool SequenceEditor::showVelocity = false;
Cell SequenceEditor::grid[32][6];
int SequenceEditor::currentStep = 0;
//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
SequenceEditor::SequenceEditor ()
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (playButton = new TextButton ("Play"));
    playButton->setButtonText (TRANS("Play"));
    playButton->addListener (this);

    playButton->setBounds (10, 10, 150, 24);

    addAndMakeVisible (recordButton = new TextButton ("Record"));
    recordButton->setButtonText (TRANS("Record"));
    recordButton->addListener (this);

    recordButton->setBounds (170, 10, 150, 24);


    //[UserPreSize]
    //[/UserPreSize]

    setSize (800, 768);

    content = new SequencePanel();
    
    
    view = new Viewport();
    view->setViewedComponent(content);

    //[Constructor] You can add your own custom stuff here..

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

    selectedCol = 0;
    selectedRow = 0;
    addAndMakeVisible(view);

    setWantsKeyboardFocus(true);
    setInterceptsMouseClicks(true,true);
    addMouseListener(content, true);
    
    //[/Constructor]
}

SequenceEditor::~SequenceEditor()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    playButton = nullptr;
    recordButton = nullptr;
    delete view;

    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void SequenceEditor::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    if (content != nullptr && view != nullptr) {
        view->setTopLeftPosition(0,50);
        view->setSize(getWidth(), cellSize * 7);
        content->setSize(cellSize*32, cellSize * 6);
    }
    //[/UserPaint]
}

void SequenceEditor::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void SequenceEditor::buttonClicked (Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]
    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == playButton)
    {
        //[UserButtonCode_receiveButton] -- add your button handler code here..

        startTimer(((float)60 / tempo) * (float)1000 / (float)4);

        //[/UserButtonCode_receiveButton]
    }
    else if (buttonThatWasClicked == recordButton)
    {
        //[UserButtonCode_sendButton] -- add your button handler code here..

        stopTimer();

        //[/UserButtonCode_sendButton]
    }

    //[UserbuttonClicked_Post]
    //[/UserbuttonClicked_Post]
}

void SequenceEditor::mouseMove (const MouseEvent& e)
{
    //[UserCode_mouseMove] -- Add your code here...

    mouseX = e.getPosition().getX();
    mouseY = e.getPosition().getY();

    /*
    for (int x = 0; x < gridWidth;x++) {
        for (int y = 0; y < gridHeight; y++) {

            if(mouseX > x * cellSize && mouseX < x * cellSize + cellSize && mouseY > y * cellSize && mouseY < y * cellSize + cellSize) {
                selectedRow = y;
                selectedCol = x;
            }
        }

    }
     */

    repaint();

    //[/UserCode_mouseMove]
}

void SequenceEditor::mouseDown (const MouseEvent& e)
{
    //[UserCode_mouseDown] -- Add your code here...


    // toggle cell
    grid[e.getPosition().getX() / cellSize][e.getPosition().getY() / cellSize].setEnabled(!grid[e.getPosition().getX() / cellSize][e.getPosition().getY() /cellSize].isEnabled());
    //[/UserCode_mouseDown]
}

void SequenceEditor::mouseDrag (const MouseEvent& e)
{
    //[UserCode_mouseDrag] -- Add your code here...

    int delta = - e.getDistanceFromDragStartY() / 20;

    if (!showVelocity) {

        int note =  grid[e.getMouseDownX()/ cellSize][e.getMouseDownY()/ cellSize].getNote();
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

        grid[e.getMouseDownX()/ cellSize][e.getMouseDownY()/ cellSize].setNote(note);

    }
    else {
        int velocity =  grid[e.getMouseDownX()/ cellSize][e.getMouseDownY()/ cellSize].getVelocity();
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

        grid[e.getMouseDownX()/ cellSize][e.getMouseDownY()/ cellSize].setVelocity(velocity);
    }

    sendConfig();

    repaint();

    //[/UserCode_mouseDrag]
}

void SequenceEditor::mouseUp (const MouseEvent& e)
{
    //[UserCode_mouseUp] -- Add your code here...
    //[/UserCode_mouseUp]
}

bool SequenceEditor::keyPressed (const KeyPress& key)
{
    //[UserCode_keyPressed] -- Add your code here...

    if (key.getKeyCode() == KeyPress::spaceKey) {
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
            note = note + 1 % 127;
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

            if (note > 0)
                note = note - 1 % 127;

            grid[selectedCol][selectedRow].setNote(note);
        }
        else {
            if(selectedRow < 5) {
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
        if(selectedCol < 63) {
            selectedCol++;
        }
    }

    sendConfig();
    repaint();

    return true;  // Return true if your handler uses this key event, or false to allow it to be passed-on.
    //[/UserCode_keyPressed]
}

void SequenceEditor::modifierKeysChanged (const ModifierKeys& modifiers)
{
    //[UserCode_modifierKeysChanged] -- Add your code here...


    optionPressed = modifiers.isAltDown();
    showVelocity = modifiers.isShiftDown();
    repaint();

    //[/UserCode_modifierKeysChanged]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...

void SequenceEditor::setDeviceManager(juce::AudioDeviceManager *manager) {
    this->deviceManager = manager;
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

        int len = decodeSysex(sysex,data+3,message.getSysExDataSize()-3);

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

    else {
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


void SequenceEditor::timerCallback() {
    currentStep = (currentStep + 1) % 32;
    content->repaint();
}

//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Projucer information section --

    This is where the Projucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="SequenceEditor" componentName=""
                 parentClasses="public Component, public MouseListener, public AudioProcessorPlayer, public Timer"
                 constructorParams="" variableInitialisers="" snapPixels="8" snapActive="1"
                 snapShown="1" overlayOpacity="0.330" fixedSize="0" initialWidth="1"
                 initialHeight="768">
  <METHODS>
    <METHOD name="mouseMove (const MouseEvent&amp; e)"/>
    <METHOD name="mouseDown (const MouseEvent&amp; e)"/>
    <METHOD name="mouseUp (const MouseEvent&amp; e)"/>
    <METHOD name="mouseDrag (const MouseEvent&amp; e)"/>
    <METHOD name="modifierKeysChanged (const ModifierKeys&amp; modifiers)"/>
    <METHOD name="keyPressed (const KeyPress&amp; key)"/>
  </METHODS>
  <BACKGROUND backgroundColour="ff323e44"/>
  <TEXTBUTTON name="receive" id="17eb77e9343032ca" memberName="receiveButton"
              virtualName="" explicitFocusOrder="0" pos="0 240 150 24" buttonText="Receive"
              connectedEdges="0" needsCallback="1" radioGroupId="0"/>
  <TEXTBUTTON name="send" id="437ee49404b1df51" memberName="sendButton" virtualName=""
              explicitFocusOrder="0" pos="160 240 150 24" buttonText="Send"
              connectedEdges="0" needsCallback="1" radioGroupId="0"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
//[/EndFile]
