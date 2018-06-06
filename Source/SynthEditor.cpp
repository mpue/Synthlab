
#include "SynthEditor.h"
#include "MidiGate.h"
#include "MidiNote.h"
#include "MidiOut.h"
#include "AudioOut.h"
#include "AudioIn.h"
#include "AuxOut.h"
#include "SawtoothModule.h"
#include "NoiseModule.h"
#include "AdderModule.h"
#include "Constant.h"
#include "PrefabFactory.h"
#include "Knob.h"
#include "ADSRModule.h"
#include <stdio.h>
#include <string.h>
#include "Actions/AddModuleAction.h"
#include "Actions/AddConnectionAction.h"
#include "Actions/DuplicateModuleAction.h"
#include "Actions/RemoveSelectedAction.h"
#include "Actions/MoveSelectedAction.h"
#include "Project.h"
#include "LabelModule.h"
#include "SamplerModule.h"
#include "OneShotTimer.h"
#include "PluginModule.h"
#include "AudioManager.h"
#include "StepSequencerModule.h"
#include "SequenceEditor.h"
#include "TerminalModule.h"
#include "OscillatorModule.h"
#include "AudioEngine/Pulse.h"
#include "AudioEngine/Sine.h"
#include "ModuleUtils.h"
#include "ExtendedFileBrowser.h"
#include "AudioRecorderModule.h"
#include "ImageModule.h"
#include "AudioRecorderEditor.h"

class SampleEditor;

String SynthEditor::defaultMidiOutputName = "Express 128 Port 7";
String SynthEditor::defaultMidiInputName = "Express 128 Port 7";

SynthEditor::SynthEditor(){
    
    setSize (1280, 800);

    root = new Module("Root");
    
    selectionModel = SelectionModel();
    
    setRepaintsOnMouseActivity(true);
    setMouseClickGrabsKeyboardFocus(true);
    //setWantsKeyboardFocus(true);
    
    addChildComponent(root);
}


SynthEditor::SynthEditor (double sampleRate, int buffersize)
{
    Logger::writeToLog("Creating SynthEditor with sample rate "+String(sampleRate)+" kHz and buffer size of "+String(buffersize)+" bytes.");
    
    this->bufferSize  = buffersize;
    this->_sampleRate = sampleRate;

    setSize (1280, 800);

    root = new Module("Root");
    
    // only the topmost module can be root
    if (Project::getInstance()->getRoot() == nullptr) {
        Project::getInstance()->setRoot(root);
    }
    
    selectionModel.setRoot(root);
    
	setRepaintsOnMouseActivity(true);
	// setMouseClickGrabsKeyboardFocus(true);
	// setWantsKeyboardFocus(true);

    addChildComponent(root);
    
    linePath = Path();
    
    Project::getInstance()->getCommandManager()->registerAllCommandsForTarget(this);
}

SynthEditor::~SynthEditor()
{
    setRunning(false);
    if (isRoot && deleteModuleWhenRemoved) {
        
        cleanUp();
        delete root;
        if (tab != nullptr) {
            tab->clearTabs();
            delete tab;
        }
        
    }

    for (int i = 0; i < openViews.size();i++) {
        delete openViews.at(i);
    }

    listener.clear();
    setRunning(true);
}

void SynthEditor::paint (Graphics& g)
{
    g.fillAll (Colour (0xff222222));
    g.setColour(juce::Colours::white);

	if (isLeftMouseDown) {

        if (state == SelectionModel::State::DRAGGING_CONNECTION) {
			// g.drawLine(lineStartX, lineStartY, lineStopX, lineStopY);
            drawCurrentConnection();
            g.fillPath(linePath);
		}
        else if (state == SelectionModel::State::DRAGGING_SELECTION) {
            g.drawRect(selectionFrame);
        }
	}
    
    if (currentLayer == Module::Layer::ALL) {
        drawConnections(g);
    }


}

void SynthEditor::drawConnections(Graphics& g) {
    for (int i = 0; i < root->getConnections()->size(); i++) {
        Connection* c = root->getConnections()->at(i);
        
        if (c->source != NULL && c->target != NULL) {
            
            if (c->selected) {
                g.setColour(juce::Colours::orange);
            }
            else {
                g.setColour(juce::Colours::lightgrey);
            }
            
            // g.drawLine(x1,y1,x2,y2);
            c->toFront(false);
            c->paint(g);
        }
        
    }
}

void SynthEditor::resized()
{
    if (getParentComponent() == nullptr) {
        return;
    }
    
    setSize(getParentWidth()*1.5, getParentHeight()*1.5);
    
    if (root != nullptr && root->getConnections() != nullptr) {
        for (int i = 0; i < root->getConnections()->size(); i++) {
            Connection* c = root->getConnections()->at(i);
            
            if (c->source != NULL && c->target != NULL) {
                
                int x1 = c->source->getX() + c->a->x;
                int y1 = c->source->getY() + c->a->y + 5;
                int x2 = c->target->getX() + c->b->x;
                int y2 = c->target->getY() + c->b->y + 5;
                
                Point<int> p1 = Point<int>(x1,y1);
                Point<int> p2 = Point<int>(x2,y2);
                c->setPoints(p1,p2);
                c->resized();
            }
        }
    }
}

void SynthEditor::drawCurrentConnection() {
    p1.x = lineStartX;
    p1.y = lineStartY;
    p2.x = lineStopX;
    p2.y = lineStopY;
    linePath.clear();
    linePath.startNewSubPath (p1.x, p1.y);
    
    linePath.cubicTo (p1.x , p1.y ,
                       p1.x + (p2.x - p1.x) * 0.3f, p1.y ,
                       p1.x + (p2.x - p1.x) * 0.5f, p1.y + (p2.y - p1.y) * 0.5f);
    linePath.cubicTo ( p1.x + (p2.x - p1.x) * 0.5f, p1.y + (p2.y - p1.y) * 0.5f,
                       p2.x - (p2.x - p1.x) * 0.3, p2.y ,
                       p2.x,p2.y  );
    PathStrokeType wideStroke (2.0f);
    wideStroke.createStrokedPath (linePath, linePath);
}

void SynthEditor::mouseMove (const MouseEvent& e)
{
	mouseX = e.getPosition().getX();
	mouseY = e.getPosition().getY();

    selectionModel.checkForConnectionSelect(e.getPosition());
}

void SynthEditor::mouseDown (const MouseEvent& e)
{
    if (locked) {
        return;
    }

    mouseDownX = e.getPosition().getX();
    mouseDownY = e.getPosition().getY();
    
    lineStartX = mouseDownX;
    lineStartY = mouseDownY;
    
    lineStopX = lineStartX;
    lineStopY = lineStartY;

	if (e.mods.isLeftButtonDown()) {
		isLeftMouseDown = true;
    }
    
    selectionModel.deselectAllPins();
    selectionModel.checkForConnectionSelect(e.getPosition());
    
    // has HitModule ?

    state = selectionModel.checkForHitAndSelect(e.getPosition(), isLeftShiftDown);
    
    if (state == SelectionModel::State::NONE) {
        state = SelectionModel::State::DRAGGING_SELECTION;
    }
    else {
        selectionModel.checkForPinSelection(e.getPosition());
        notifyListeners();
    }
    
    // has any connection been clicked?
    
    selectionModel.checkForConnectionSelect(e.getPosition());
	
	if (e.mods.isRightButtonDown()) {
        showContextMenu(e.getPosition());
    }


    
    if (root->getModules()->size() > 0) {
        sendChangeMessage();
        notifyListeners();
    }
    repaint();

}

void SynthEditor::mouseDrag (const MouseEvent& e)
{
    
    if (locked) {
        return;
    }
    
    dragHasStarted = true;
    
    mouseX = e.getPosition().getX();
    mouseY = e.getPosition().getY();
    
    dragDistanceX = e.getDistanceFromDragStartX();
    dragDistanceY = e.getDistanceFromDragStartY();
    
    lineStopX = lineStartX + e.getDistanceFromDragStartX();
    lineStopY = lineStartY + e.getDistanceFromDragStartY();
    
    if (state != SelectionModel::State::DRAGGING_SELECTION) {
        checkForConnectionDrag();
    }
    
    if (state == SelectionModel::State::MOVING_SELECTION) {
        moveSelection(e);
    }
    else {
        for (int i = 0; i < root->getModules()->size(); i++) {
            Module* m = root->getModules()->at(i);
            if (!m->isSelected()) {
                selectionModel.checkForPinSelection(e.getPosition());
            }
        }
    }
    
    if (state == SelectionModel::State::DRAGGING_SELECTION) {
        dragSelection();
    }
    
    repaint();
    resized();
}

void SynthEditor::checkForConnectionDrag() {
    for (int i = 0; i < root->getModules()->size(); i++) {
        
        Module* m = root->getModules()->at(i);
        
        if (m->isSelected()) {
            for (int j = 0; j < m->pins.size(); j++) {
                if (m->pins.at(j)->isSelected()) {
                    lineStartX = m->getX() + m->pins.at(j)->x + 5;
                    lineStartY = m->getY() + m->pins.at(j)->y + 5;
                    startPin = m->pins.at(j);
                    state = SelectionModel::State::DRAGGING_CONNECTION;
                    break;
                }
                
            }
        }
    }
}

void SynthEditor::moveSelection(const MouseEvent& e) {
    for (int i = 0; i < root->getModules()->size(); i++) {
        
        Module* m = root->getModules()->at(i);
        
        if (m->isSelected()) {
            
            if (m->getSelectedPin() == nullptr || m->getSelectedPin() == NULL ) {
                
                if (currentLayer == Module::Layer::GUI) {
                    
                    int x = m->getSavedUiPosition().getX() + e.getOffsetFromDragStart().getX();
                    int y = m->getSavedUiPosition().getY()+ e.getOffsetFromDragStart().getY();
                    
                    if (snapToGrid) {
                        x = snap(x,10);
                        y = snap(y,10);
                        
                    }
                    
                    m->setUiPosition(x,y);
                    m->setTopLeftPosition(x,y);
                }
                else {
                    
                    int x = m->getSavedPosition().getX() + e.getOffsetFromDragStart().getX();
                    int y = m->getSavedPosition().getY() + e.getOffsetFromDragStart().getY();
                    
                    if (snapToGrid) {
                        x = snap(x,10);
                        y = snap(y,10);
                        
                    }
                    
                    m->setTopLeftPosition(x,y);
                    
                }
                
                repaint();
            }
        }
    }
}

void SynthEditor::dragSelection() {
    int x = mouseDownX;
    int y = mouseDownY;
    
    if (mouseX < x) {
        x = mouseX;
    }
    if (mouseY < y) {
        y = mouseY;
    }
    selectionFrame.setBounds(x,y,abs(dragDistanceX),abs(dragDistanceY));
    
    for (int i = 0; i < root->getModules()->size(); i++) {
        Module* m = root->getModules()->at(i);
        
        if (selectionFrame.contains(m->getBounds())) {
            if (!m->isSelected()) {
                m->setSelected(true);
                if (currentLayer == Module::Layer::GUI) {
                    m->saveUiPosition();
                }
                else {
                    m->savePosition();
                }
                selectionModel.getSelectedModules().push_back(m);
            }
            
        }
    }
}

void SynthEditor::mouseUp (const MouseEvent& e)
{
    if (e.mods.isLeftButtonDown()) {
        isLeftMouseDown = false;
    }
    
    if (state == SelectionModel::MOVING_SELECTION) {
        MoveSelectedAction* msa = new MoveSelectedAction(this);
        Project::getInstance()->getUndoManager()->beginNewTransaction();
        Project::getInstance()->getUndoManager()->perform(msa);
    }
    
    else if (state == SelectionModel::State::DRAGGING_CONNECTION)  {
        for (int i = 0; i < root->getModules()->size(); i++) {
            
            Module* m = root->getModules()->at(i);
            
            if (m->isSelected()) {
                
                addConnection(e, m);
            }
        }
    }
    
    lineStopX = 0;
    lineStopY = 0;
    
    state = SelectionModel::State::NONE;
    
    dragHasStarted = false;
}

void SynthEditor::mouseDoubleClick (const MouseEvent& e)
{
    if (isAltDown) {
        for (int i = 0; i < root->getModules()->size(); i++) {
            
            Module* m = root->getModules()->at(i);
            
            if (m->isSelected()) {
                m->setEditing(true);
                break;
            }
        }
    }
    else {
        for (int i = 0; i < root->getModules()->size(); i++) {
            
            Module* m = root->getModules()->at(i);
            
            if (m->isPrefab() && m->getBounds().contains(e.getPosition())) {
                
                SamplerModule*sm = dynamic_cast<SamplerModule*>(m);
                
                if (sm != NULL) {
                    openSampleEditor(sm);
                }
                
                PluginModule *pm = dynamic_cast<PluginModule*>(m);
                
                if (pm != NULL) {
                    pm->openPluginWindow();
                }
                
                StepSequencerModule *ssm = dynamic_cast<StepSequencerModule*>(m);
                
                if (ssm != NULL) {
                    openStepSequencer(ssm);
                }
                
                AudioRecorderModule *arm = dynamic_cast<AudioRecorderModule*>(m);
                
                if (arm != NULL) {
                    openRecorder(arm);
                }
                
            }
            else {
                if (m->isSelected() && m->isEditable()) {
                    openEditor(m);
                }
            }
        }
    }
    /*
     if (selectionModel.getSelectedModules().size() == 0) {
     showContextMenu(e.getPosition());
     }
     */
    //[/UserCode_mouseDoubleClick]
}

/**
 * Snaps the current given location to a given grid
 *
 * @param location the location to be snapped
 * @param raster the raster size
 * @param tolerance the tolerance to cosult while snapping
 * @return
 */
int SynthEditor::snap(int location, int raster) {
    
    int toleranceWindow = (raster / 2);
    
    if (location > 0) {
        if ((location % raster) > toleranceWindow) {
            location = location + (raster - (location % raster));
        }
        else {
            location = location - (location % raster);
        }
    }
    else {
        if ((location % raster) < toleranceWindow) {
            location = location + (raster - (location % raster)) - raster;
        }
        else {
            location = location - (location % raster) - raster;
        }
    }
    return location;
}

void SynthEditor::showContextMenu(Point<int> position) {
    PopupMenu* m = new PopupMenu();
    m->setLookAndFeel(Project::getInstance()->getLookAndFeel());
    
    Module* module = nullptr;

    if (selectionModel.getSelectedModule() != nullptr) {
        module = selectionModel.getSelectedModule();
    }

    if (module != nullptr) {

        int pinIndex = -1;
        
        for (int i = 0; i < module->getPins().size();i++) {
            if (module->getPins().at(i)->isSelected()) {
                pinIndex = i;
            }
        }
        
        Knob* k;
        
        if(pinIndex < 0 ) {
            if (module->isEditable()) {
                m->addItem(1, "Add input");
                m->addItem(2, "Add output");
                m->addItem(4, "Duplicate");
                m->addItem(6, "Save module");

            }
            else {
                if ((k = dynamic_cast<Knob*>(module)) != NULL) {
                    m->addItem(3, "MIDI learn");
                    m->addCommandItem(Project::getInstance()->getCommandManager(), SynthEditor::CommandIds::RESET_GUI_POS);
                }
            }
      
            const int result = m->show();
            
            if (result == 0)
            {
                // user dismissed the menu without picking anything
            }
            else if (result == 1) {
                module->addPin(Pin::Direction::IN);
                
            }
            else if (result == 2) {
                module->addPin(Pin::Direction::OUT);
            }
            
            else if (result == 3) {
                k->setLearning(true);
            }
            else if (result == 4) {
                duplicateSelected();
            }
            else if (result == 6) {
                saveModule(selectionModel.getSelectedModule());
            }
        }
        else {
            m->addItem(1, "Add control");
            m->addItem(2, "Add constant");
            m->addItem(3, "Add terminal");
            
            const int result = m->show();
            
            if (result == 0)
            {
                // user dismissed the menu without picking anything
            }
            else if (result == 1) {
                addControl(module, pinIndex);
            }
            else if (result == 2) {
                addConstant(module, pinIndex);
            }
            else if (result == 3) {
                addTerminal(module, pinIndex);
            }
            
        }
        
    }
    else {
        
        m->addItem(1,"Add module");
        // m->addCommandItem(Project::getInstance()->getCommandManager(), CommandIds::ADD_MODULE);
        m->addCommandItem(Project::getInstance()->getCommandManager(), CommandIds::DELETE_SELECTED);
        // m->addCommandItem(Project::getInstance()->getCommandManager(), CommandIds::LOAD_MODULE);
        m->addItem(3,"Load module");
        m->addCommandItem(Project::getInstance()->getCommandManager(), CommandIds::SAVE_MODULE);
        m->addCommandItem(Project::getInstance()->getCommandManager(), CommandIds::SAVE_SCREENSHOT);
        
        if (locked) {
            m->addItem(99, "Unlock");
            for (int i = 0; i < root->getModules()->size(); i++) {
                // root->getModules()->at(i)->setInterceptsMouseClicks(false, true);
            }
        }
        else {
            for (int i = 0; i < root->getModules()->size(); i++) {
                // root->getModules()->at(i)->setInterceptsMouseClicks(true, true);
            }
             m->addItem(99, "Lock");
        }
        
        
        PopupMenu alignMenu = PopupMenu();
        
        alignMenu.addCommandItem(Project::getInstance()->getCommandManager(), CommandIds::ALIGN_X);
        alignMenu.addCommandItem(Project::getInstance()->getCommandManager(), CommandIds::ALIGN_Y);
        
        m->addSubMenu("Align",alignMenu);
        
        PopupMenu prefabMenu = PopupMenu();
        
        StringArray* categories = PrefabFactory::getInstance()->getCategories();
        
        for (int i = 0; i < categories->size();i++) {
            PopupMenu category = PopupMenu();
            
            std::map<int,PrefabFactory::Prefab> prefabs = PrefabFactory::getInstance()->getPrefabNames();
            
            for (std::map<int,PrefabFactory::Prefab>::iterator it  = prefabs.begin();it != prefabs.end();++it) {
                if ((*it).second.getCategory() == categories->getReference(i)) {

                    if (((*it).second.getName().equalsIgnoreCase("Audio In") || (*it).second.getName().equalsIgnoreCase("Audio Out")) && !isRoot) {
                        continue;
                    }
                    
                    if ((*it).second.getIcon() != nullptr) {
                        // int itemResultID, const String& itemText, bool isActive, bool isTicked, const Image& iconToUs
                        category.addItem((*it).first,(*it).second.getName(), true,false,*(*it).second.getIcon());
                    }
                    else {
                        category.addItem((*it).first,(*it).second.getName(), true);
                    }
                 
                }
                
            }
            
            prefabMenu.addSubMenu(categories->getReference(i), category, true);
        }
        
        m->addSubMenu("Prefabs",prefabMenu);
        
        PopupMenu recentFiles = PopupMenu();
        
        Project::getInstance()->loadRecentFileList();
        StringArray recent = Project::getInstance()->getRecentFiles();
        
        for (int i = 0; i < recent.size();i++) {
            recentFiles.addItem(i+1000,recent.getReference(i));
        }
        
        m->addSubMenu("Recent files", recentFiles);
        
        const int result = m->show();
        
        if (result == 0)
        {
            // user dismissed the menu without picking anything
        }
        else if (result == 1) {
            Module* m = new Module("Module");
            m->setIndex(PrefabFactory::getInstance()->getNextModuleIndex());
            AddModuleAction* am = new AddModuleAction(this,position,m->getIndex());
            am->setModule(m);
            Project::getInstance()->getUndoManager()->beginNewTransaction();
            Project::getInstance()->getUndoManager()->perform(am);
        }
        else if (result == 3) {
            setRunning(false);
            Module* m = loadModule();
            setRunning(true);
        }
        else if (result == 99) {
            locked = !locked;
        }
        else if (result >= 30 && result <= 99 ){
            AddModuleAction* am = new AddModuleAction(this,position,result);
            Project::getInstance()->getUndoManager()->beginNewTransaction();
            Project::getInstance()->getUndoManager()->perform(am);
        }
        else if (result >= 1000) {
            StringArray recent = Project::getInstance()->getRecentFiles();
            String path = recent.getReference(result - 1000);
            File file = File(path);
            
            if (file.exists()) {
                FileInputStream *fis = new FileInputStream(file);
                String data = fis->readEntireStreamAsString();
                delete fis;
                setRunning(false);
                cleanUp();
                newFile();
                loadFromString(data);
                updateProject(file);
                Project::getInstance()->setNewFile(false);
                setRunning(true);
            }

        }

    }
    
    m->setLookAndFeel(nullptr);
    delete m;

}

void SynthEditor::connect(Module* root, Module* c, Module* module , int pinIndex, int mouseY ) {
    
    Point<int> pos = module->getPinPosition(pinIndex);
    c->setTopLeftPosition(pos.getX() - 150, mouseY - c->getHeight() / 2);
    
    module->pins.at(pinIndex)->getConnections().push_back(c->getPins().at(0));
    addAndMakeVisible(c);
    
    Connection* con = new Connection();
    con->a = c->getPins().at(0);
    con->b = module->pins.at(pinIndex);
    con->source = c;
    con->target = module;
    
    root->getConnections()->push_back(con);
    root->getModules()->push_back(c);
    
    repaint();
    resized();
    
}
void SynthEditor::addControl(Module* module, int pinIndex) {
    Knob* k = dynamic_cast<Knob*>(PrefabFactory::getInstance()->getPrefab(69, _sampleRate, bufferSize));
    addChangeListener(k);
    k->setValue(1);
    k->setName(module->getPins().at(pinIndex)->getName());
    
    connect(root, k, module, pinIndex, mouseY);
    
    selectionModel.clearSelection();
    k->setSelected(true);
    
    if (module->getName().contains("Filter")) {
        k->setMinimum(100);
        k->setMaximum(15000);
        k->setStepSize(10);
    }
    if (module->getName().contains("Mixer")){
        k->setMinimum(0);
        k->setMaximum(1);
        k->setStepSize(0.01);
    }
    if (module->getName().contains("ADSR")){
        
        k->setMinimum(0);
        
        if (module->getPins().at(pinIndex)->getName() == "S"){
            k->setMaximum(1);
        }
        else {
            k->setMaximum(10);
        }
        
        k->setStepSize(0.01);
    }
    
    selectionModel.getSelectedModules().push_back(k);
    repaint();
    resized();
}

void SynthEditor::addConstant(Module *module, int pinIndex) {
    Constant* c = dynamic_cast<Constant*>(PrefabFactory::getInstance()->getPrefab(67, _sampleRate, bufferSize));
    addChangeListener(c);
    c->setValue(1);
    c->setName(module->getPins().at(pinIndex)->getName());
    connect(root, c, module, pinIndex, mouseY);

}

void SynthEditor::addTerminal(Module *module, int pinIndex) {
    
    Pin* targetPin = module->pins.at(pinIndex);
    
    TerminalModule* t = nullptr;

    if (targetPin->getDirection() == TerminalModule::Direction::IN) {
        t = dynamic_cast<TerminalModule*>(PrefabFactory::getInstance()->getPrefab(83, getSampleRate(), getBufferSize()));
       
    }
    else {
        t = dynamic_cast<TerminalModule*>(PrefabFactory::getInstance()->getPrefab(84, getSampleRate(), getBufferSize()));
        
    }
    
    t->setType(targetPin->getType());
    Pin* p = new Pin(targetPin->getType());
    t->getPins().at(0)->setTerminal(p);
    p->listeners.push_back(t);
    p->setName(t->getName());
    t->addChangeListener(p);
    if (t->getDirection() == TerminalModule::Direction::IN) {
        getModule()->addPin(Pin::Direction::IN, p);
    }
    else {
        getModule()->addPin(Pin::Direction::OUT, p);
    }
    t->setIndex(p->index);
    
    // ModuleUtils::connectModules(t, module, pinIndex);
    
    connect(root, t, module, pinIndex, mouseY);
    
    addAndMakeVisible(t);
    
    getModule()->repaint();
    
}

void SynthEditor::itemDropped (const SourceDetails& dragSourceDetails)  {
    AddModuleAction* am = new AddModuleAction(this,dragSourceDetails.localPosition,dragSourceDetails.description.toString().getIntValue());
    Project::getInstance()->getUndoManager()->beginNewTransaction();
    Project::getInstance()->getUndoManager()->perform(am);
};



float SynthEditor::getSampleRate() {
    return _sampleRate;
}

float SynthEditor::getBufferSize() {
    return bufferSize;
}

bool SynthEditor::isRunning() {
    return running;
    //stopTimer();
}

void SynthEditor::setRunning(bool running) {
    this->running = running;
    
    CriticalSection& c = AudioManager::getInstance()->getDeviceManager()->getAudioCallbackLock();
    
    if (!running) {
       
        c.enter();
    }
    else {
        c.exit();
    }
    // startTimer(100);
}

void SynthEditor::timerCallback(){
    repaint();
}

void SynthEditor::setCurrentLayer(int layer) {
    
    currentLayer = static_cast<Module::Layer>(layer);
    
    selectionModel.setCurrentLayer(layer);
   
    for (int i = 0;i < root->getModules()->size();i++) {
        root->getModules()->at(i)->setCurrentLayer(currentLayer);
        if (currentLayer == Module::Layer::ALL || root->getModules()->at(i)->getLayer() == currentLayer) {
            
            if (currentLayer == Module::Layer::GUI ) {
                root->getModules()->at(i)->setTopLeftPosition(root->getModules()->at(i)->getUiPosition().x, root->getModules()->at(i)->getUiPosition().y);
                root->getModules()->at(i)->saveUiPosition();
            }
            else {
                root->getModules()->at(i)->setTopLeftPosition(root->getModules()->at(i)->getSavedPosition().x, root->getModules()->at(i)->getSavedPosition().y);
                root->getModules()->at(i)->savePosition();
            }
            
            root->getModules()->at(i)->setVisible(true);
        }
        else {
            root->getModules()->at(i)->setVisible(false);
        }
        
       
    }
    repaint();
    resized();
}

int SynthEditor::getCurrentLayer() {
    return static_cast<int>(currentLayer);
}

SelectionModel& SynthEditor::getSelectionModel() {
    return selectionModel;
}

void SynthEditor::setMixer(MixerPanel* mixer) {
    this->mixer = mixer;
}

MixerPanel* SynthEditor::getMixer() {
    return mixer;
}

bool SynthEditor::keyPressed (const KeyPress& key)
{
    if (key.getKeyCode() == KeyPress::deleteKey || key.getKeyCode() == KeyPress::backspaceKey) {
        RemoveSelectedAction* rsa = new RemoveSelectedAction(this);
        Project::getInstance()->getUndoManager()->beginNewTransaction();
        Project::getInstance()->getUndoManager()->perform(rsa);
    }
    if(key.getKeyCode() == 65 && isCtrlDown) {
        for (int i = 0; i < root->getModules()->size();i++) {
            root->getModules()->at(i)->setSelected(true);
            selectionModel.getSelectedModules().push_back(root->getModules()->at(i));
        }
    }
    
    repaint();
    return true;
}

bool SynthEditor::keyStateChanged (bool isKeyDown)
{
    return false;
}

void SynthEditor::modifierKeysChanged (const ModifierKeys& modifiers)
{
    isAltDown = modifiers.isAltDown();
    isCtrlDown = modifiers.isCtrlDown() ||  modifiers.isCommandDown();
    isLeftShiftDown = modifiers.isShiftDown();
}

void SynthEditor::cleanUp() {
    mixer->removeAllChannels();
    for(std::vector<Module*>::iterator it = root->getModules()->begin();it != root->getModules()->end();it++) {
        ModuleUtils::removeModule(root, (*it),getMixer(), this);
    }
    for(std::vector<Module*>::iterator it = root->getModules()->begin();it != root->getModules()->end();) {
        for(std::vector<Connection*>::iterator it2 = (*it)->getConnections()->begin();it2 != (*it)->getConnections()->end();) {
            delete *it2;
            it2 = (*it)->getConnections()->erase(it2);
        }
        delete *it;
        it = root->getModules()->erase(it);
    }
    for(std::vector<Connection*>::iterator it = root->getConnections()->begin();it != root->getConnections()->end();) {
        delete *it;
        it = root->getConnections()->erase(it);
    }
    
    PrefabFactory::getInstance()->reset();
    
    delete root;
    root = nullptr;
    //deleteSelected(true);
    selectionModel.getSelectedModules().clear();
    mixer->clearChannels();
    Project::getInstance()->getUndoManager()->clearUndoHistory();
    removeAllChangeListeners();
    if (tab != nullptr) {
        
        int i = tab->getNumTabs();
        
        while(tab->getNumTabs() > 1) {
            tab->removeTab(--i);
        }
    }
    
    if (tab != nullptr)
        tab->setCurrentTabIndex(0);
    
    if (Project::getInstance()->getAppMode() == Project::AppMode::STUDIO) {
        MainTabbedComponent* supplemental = Project::getInstance()->getSupplemental();
        
        if (supplemental != nullptr) {
            
            int i = supplemental->getNumTabs();
            
            while(supplemental->getNumTabs() > 1) {
                supplemental->removeTab(--i);
            }
        }
        supplemental->setCurrentTabIndex(0);
    }
    

}

void SynthEditor::newFile() {
    root = new Module("Root");
    selectionModel = SelectionModel();
    selectionModel.setRoot(root);
    setLocked(false);
    updateProject(File());
    repaint();
}


void SynthEditor::loadFromString(juce::String in){
    
    ScopedPointer<XmlElement> xml = XmlDocument(in).getDocumentElement();
    
    ValueTree v = ValueTree::fromXml(*xml.get());

    setRunning(false);
    ModuleUtils::loadStructure(root->getModules(),root->getConnections(), &v, this);
    
    for (std::vector<Module*>::iterator it = root->getModules()->begin(); it != root->getModules()->end(); ++it) {
        addAndMakeVisible((*it));
        configureAudioModule((*it),this);
        ImageModule* im;
        if ((im = dynamic_cast<ImageModule*>(*it)) != NULL) {
            im->toBack();
        }
    }
    
    setCurrentLayer(v.getProperty("layer").toString().getIntValue());
    setLocked(v.getProperty("lock").toString().getIntValue() > 0);
    
    notifyListeners();
    
    xml = nullptr;
    resized();
    
    setRunning(true);
}

void SynthEditor::saveModule(Module* m) {

    FileChooser chooser("Select target file...", File(), "*");
    
    if (chooser.browseForFileToSave(true)) {
        
        ValueTree* v = new ValueTree("Module");
        v->setProperty("name", m->getName(), nullptr);
        
        ModuleUtils::savePins(m, *v);
        ModuleUtils::saveStructure(m->getModules(),m->getConnections(), v);
        ModuleUtils::updateIndices(*v, Time::getMillisecondCounter());
        
        File file = chooser.getResult();
        
        XmlElement* xml = v->createXml();
        xml->writeToFile(file, "");
        
        delete xml;
        delete v;
        
    }
    
}

Module* SynthEditor::loadModule() {
    FileChooser chooser("Select module to open", File(), "*");

    if (chooser.browseForFileToOpen()) {
        
        File file = chooser.getResult();
        ScopedPointer<XmlElement> xml = XmlDocument(file).getDocumentElement();
        
        Project::getInstance()->addRecentFile(file.getFullPathName());
         ValueTree v = ValueTree::fromXml(*xml.get());

        setRunning(false);

        Module* module = ModuleUtils::loadFromXml(v, this);
        module->setName(v.getProperty("name"));
        module->setTopLeftPosition(mouseX,mouseY);
        
        this->addAndMakeVisible(module);
        this->getModule()->getModules()->push_back(module);
        
        module->setSelected(true);
        module->savePosition();
        getSelectionModel().getSelectedModules().push_back(module);
        repaint();
        setRunning(true);
        xml = nullptr;
        return module;
    }
    return nullptr;
}


void SynthEditor::saveFile() {
    
#if JUCE_IOS
    File path = File::getSpecialLocation(File::userApplicationDataDirectory);
    File outputDir = File(path.getFullPathName()+"/structure.xml");
    
    URL file;
    
    ValueTree* v = new ValueTree("Synth");
    
    ModuleUtils::saveStructure(root->getModules(),root->getConnections(), v);
    
    v->setProperty("lock", isLocked(), nullptr);
    v->setProperty("layer",static_cast<int>(currentLayer),nullptr);
    
    if (Project::getInstance()->isNewFile()) {
        
        FileChooser chooser("Select target file...",outputDir, "*");

        if (chooser.browseForFileToSave(true)) {
            file = chooser.getURLResult();
        }
        
    }
    else {
        file = new URL(Project::getInstance()->getCurrentFilePath());
    }
    
    OutputStream* os = file.createOutputStream();
    XmlElement* xml = v->createXml();
    xml->writeToStream(*os, "");
    
    delete os;
    delete xml;
    delete v;

#else
    
    bool fileValid = false;
    
    File file;
    
    ValueTree* v = new ValueTree("Module");
    
    v->setProperty("lock", isLocked(), nullptr);
    v->setProperty("layer",static_cast<int>(currentLayer), nullptr);
    
    ModuleUtils::saveStructure(root->getModules(),root->getConnections(), v);
    
    if (Project::getInstance()->isNewFile()) {
        
        FileChooser chooser("Select target file...", File(), "*");
        
        if (chooser.browseForFileToSave(true)) {
            file = chooser.getResult();
            fileValid = true;
        }
        
    }
    else {
        file = File(Project::getInstance()->getCurrentFilePath());
        if (file.exists()) {
            fileValid = true;
        }
    }
    
    if (fileValid) {
        XmlElement* xml = v->createXml();
        xml->writeToFile(file, "");
        
        delete xml;
        delete v;
        
        setDirty(false);
        
        Project::getInstance()->setNewFile(false);
        updateProject(file);
    }
    


#endif

}

void SynthEditor::openEditor(Module *m) {
    
    for (int i = 0; i < tab->getNumTabs();i++) {
        
        Viewport* view = dynamic_cast<Viewport*>(tab->getTabContentComponent(i));
        
        if (view != nullptr) {
            
            SynthEditor* editor = dynamic_cast<SynthEditor*>(view->getViewedComponent());
            
            if (editor != nullptr) {
                if (editor->getModule() != nullptr && editor->getModule() == m) {
                    tab->setCurrentTabIndex(i);
                    return;
                }
            }
           

        }
        
    }
    
    SynthEditor* editor = new SynthEditor(_sampleRate, bufferSize);
    Viewport* editorView = new Viewport();
    editorView->setSize(500,200);
    editorView->setViewedComponent(editor);
    editorView->setScrollBarsShown(true,true);
    editorView->setScrollOnDragEnabled(false);
    editorView->setWantsKeyboardFocus(false);
    editorView->setMouseClickGrabsKeyboardFocus(false);
    editor->setModule(m, false);
    editor->setIndex(static_cast<int>(m->getIndex()));
    editor->setTab(tab);
    editor->addEditorListener(Project::getInstance()->getPropertyView());
    tab->addTab(m->getName(), Colours::darkgrey, editorView, false);
    openViews.push_back(editorView);
    tab->setCurrentTabIndex(tab->getNumTabs() - 1);
    editor->resized();
}

void SynthEditor::openSampleEditor(SamplerModule *sm) {
    
    MainTabbedComponent* tab = Project::getInstance()->getSupplemental();
    
    for (int i = 0; i < tab->getNumTabs();i++) {
        
        SampleEditor* editor = dynamic_cast<SampleEditor*>(tab->getTabContentComponent(i));
        
        if (editor != nullptr) {
            if (editor != nullptr && editor->getModule() == sm) {
                tab->setCurrentTabIndex(i);
                return;
            }
        }
        
    }
    
    Project::getInstance()->getSupplemental()->addTab("Sample editor", Colours::darkgrey, sm->getEditor(), false);
    Project::getInstance()->getSupplemental()->setCurrentTabIndex(Project::getInstance()->getSupplemental()->getNumTabs() - 1);

}

void SynthEditor::openStepSequencer(StepSequencerModule *ssm) {
    SequenceEditor* se = ssm->getEditor();
    Project::getInstance()->getSupplemental()->addTab("Step sequencer", Colours::darkgrey, se, true);
    Project::getInstance()->getSupplemental()->setCurrentTabIndex(Project::getInstance()->getSupplemental()->getNumTabs() - 1);
}

void SynthEditor::openRecorder(AudioRecorderModule *arm) {
    
    MainTabbedComponent* tab = Project::getInstance()->getSupplemental();
    
    for (int i = 0; i < tab->getNumTabs();i++) {
        
        AudioRecorderEditor* editor = dynamic_cast<AudioRecorderEditor*>(tab->getTabContentComponent(i));
        
        if (editor != nullptr) {
            if (editor != nullptr && editor->getIndex() == arm->getIndex()) {
                tab->setCurrentTabIndex(i);
                return;
            }
        }
        
    }
    AudioRecorderEditor* are = arm->getEditor();
    are->setIndex(static_cast<int>(arm->getIndex()));
    Project::getInstance()->getSupplemental()->addTab("Audio recorder", Colours::darkgrey, are, false);
    Project::getInstance()->getSupplemental()->setCurrentTabIndex(Project::getInstance()->getSupplemental()->getNumTabs() - 1);
}

void SynthEditor::openFile() {
    FileChooser chooser("Select file to open", File::getSpecialLocation(File::commonDocumentsDirectory), "*",true, true);

    if (chooser.browseForFileToOpen()) {
        cleanUp();
        newFile();
        
#if JUCE_IOS
        URL url = chooser.getURLResult();
        InputStream* is = url.createInputStream(false);
        String data = is->readEntireStreamAsString();
        delete is;
        
        ScopedPointer<XmlElement> xml = XmlDocument(data).getDocumentElement();
#else
        File file = chooser.getResult();
        ScopedPointer<XmlElement> xml = XmlDocument(file).getDocumentElement();
        
        Project::getInstance()->addRecentFile(file.getFullPathName());
#endif
        ValueTree v = ValueTree::fromXml(*xml.get());
        setRunning(false);
        ModuleUtils::loadStructure(root->getModules(),root->getConnections(), &v, this);
        
        for (std::vector<Module*>::iterator it = root->getModules()->begin(); it != root->getModules()->end(); ++it) {
            addAndMakeVisible((*it));
            configureAudioModule((*it),this);
            ImageModule* im;
            if ((im = dynamic_cast<ImageModule*>(*it)) != NULL) {
                im->toBack();
            }
        }

        resized();

        setCurrentLayer(v.getProperty("layer").toString().getIntValue());
        setLocked(v.getProperty("lock").toString().getIntValue() > 0);
        
        notifyListeners();
        
        xml = nullptr;
        
        updateProject(file);
        
        setRunning(true);
    }
}



void SynthEditor::configureAudioModule(Module *m, ChangeBroadcaster* broadcaster) {
    AudioManager* am = AudioManager::getInstance();
    
    AudioOut* out;
    
    if ((out = dynamic_cast<AudioOut*>(m)) != NULL) {
        mixer->getOutputChannels().push_back(out);
        String channelName = am->getOutputChannelNames().getReference(static_cast<int>(mixer->getOutputChannels().size()) - 1);
        int channelIndex = addChannel(channelName, Mixer::Channel::Type::OUT);
        out->setChannelIndex(channelIndex);
        mixer->setVolume(channelIndex, out->getVolume());
        mixer->setModule(channelIndex, out);
    }
    
    AudioIn* in;
    
    if ((in = dynamic_cast<AudioIn*>(m)) != NULL) {
        if (am->getInputChannelNames().size() > 0) {
            mixer->getInputChannels().push_back(in);
            String channelName = am->getInputChannelNames().getReference(static_cast<int>(mixer->getInputChannels().size()) - 1);
            int channelIndex = addChannel(channelName, Mixer::Channel::Type::IN);
            in->setChannelIndex(channelIndex);
            mixer->setVolume(channelIndex, in->getVolume());
            mixer->setModule(channelIndex, in);
        }

    }
    
    AuxOut* aux;
    
    if ((aux = dynamic_cast<AuxOut*>(m)) != NULL) {
        mixer->getAuxChannels().push_back(aux);
        String channelName = "Aux "+ String(mixer->getAuxChannels().size());
        int channelIndex = addChannel(channelName, Mixer::Channel::Type::AUX);
        aux->setChannelIndex(channelIndex);
        mixer->setVolume(channelIndex, aux->getVolume());
        mixer->setModule(channelIndex, aux);
    }
    
    broadcaster->addChangeListener(m);
    

}

void SynthEditor::setTab(juce::TabbedComponent *t) {
    this->tab = t;
}

void SynthEditor::addConnection(const MouseEvent& e, Module* source) {
    AddConnectionAction* ac = new AddConnectionAction(this,source);
    Project::getInstance()->getUndoManager()->beginNewTransaction();
    Project::getInstance()->getUndoManager()->perform(ac);
    resized();
    repaint();
}

void SynthEditor::setModule(Module *m, bool deleteWhenRemoved) {
    
    if (root != nullptr) {
        delete root;
    }
    
    this->root = m;

    selectionModel.setRoot(m);
    isRoot = false;
    this->deleteModuleWhenRemoved = deleteWhenRemoved;
    for (std::vector<Module*>::iterator it = root->getModules()->begin(); it != root->getModules()->end(); ++it) {
        addAndMakeVisible((*it));
    }

    repaint();
}

Module* SynthEditor::getModule() {
    return root;
}



void SynthEditor::prepareToPlay(int samplesPerBlockExpected, double sampleRate) {
    this->_sampleRate = sampleRate;
    this->bufferSize = samplesPerBlockExpected;
    
    Logger::writeToLog("SynthEditor using sample rate "+String(sampleRate)+" kHz and buffer size of "+String(samplesPerBlockExpected)+" bytes.");
    
    sendChangeMessage();
   
}

bool SynthEditor::isInterestedInDragSource (const SourceDetails& dragSourceDetails) {
    return true;
}

void SynthEditor::setSamplerate(double rate) {
    this->_sampleRate = rate;
}

void SynthEditor::setBufferSize(int buffersize) {
    this->bufferSize = buffersize;
}



int SynthEditor::addChannel(juce::String name, Mixer::Channel::Type channeltype) {
    return mixer->addChannel(name, channeltype);
    
}

void SynthEditor::duplicateSelected() {
    Module* m = selectionModel.getSelectedModule();
    if (m != nullptr) {
        Point<int> pos = m->getPosition();
        pos.addXY(10, m->getBounds().getY() + 10);
        DuplicateModuleAction* am = new DuplicateModuleAction(this,pos,m);
        Project::getInstance()->getUndoManager()->beginNewTransaction();
        Project::getInstance()->getUndoManager()->perform(am);
    }
}

void SynthEditor::addEditorListener(EditorListener *listener) {
    this->listener.push_back(listener);
}

void SynthEditor::notifyListeners() {
    
    if (this->selectionModel.getSelectedModule() != nullptr) {
    
        for (int i = 0;i < listener.size();i++) {
            listener.at(i)->selectionChanged(this->selectionModel.getSelectedModule());
        }
    }
    

    
    
}

void SynthEditor::notifyDirtyListeners() {
    for (int i = 0;i < listener.size();i++) {
        listener.at(i)->dirtyChanged(isDirty());
    }
}

bool SynthEditor::isDirty() {
    return dirty;
}

void SynthEditor::setDirty(bool dirty) {
    this->dirty = dirty;
    for (int i = 0; i < tab->getNumTabs();i++) {
        
        Viewport* view = dynamic_cast<Viewport*>(tab->getTabContentComponent(i));
        
        if (view != nullptr) {
            
            SynthEditor* editor = dynamic_cast<SynthEditor*>(view->getViewedComponent());
            
            if (editor != nullptr && editor == this) {
                
                if (dirty) {
                    if (Project::getInstance()->isNewFile()) {
                        tab->setTabName(i, "untitled*");
                    }
                    else {
                        tab->setTabName(i, Project::getInstance()->getCurrentFileName()+"*");
                    }
                }
                else {
                    tab->setTabName(i, Project::getInstance()->getCurrentFileName());
                }
            }
        }
    }
    notifyDirtyListeners();
    Project::getInstance()->setDirty(dirty);
}

int SynthEditor::getIndex() {
    return index;
}

void SynthEditor::setIndex(int index) {
    this->index = index;
}

void SynthEditor::saveScreenShot() {
    Image image = createComponentSnapshot(getLocalBounds());
    JPEGImageFormat format;
    
    FileChooser chooser("Select target file...", File(), "*");
    
    if (chooser.browseForFileToSave(true)) {
        FileOutputStream* fos = new FileOutputStream(chooser.getResult());
        format.writeImageToStream (image, *fos);
        delete fos;
    }
}

void SynthEditor::changeListenerCallback(juce::ChangeBroadcaster *source) {
    
    ExtendedFileBrowser* efb = dynamic_cast<ExtendedFileBrowser*>(source);
    
    if (efb != nullptr) {
        
        File* f = efb->getSelectedFile();
        
        if (f->getFullPathName().endsWith("slb") || f->getFullPathName().endsWith("xml")) {
            FileInputStream *fis = new FileInputStream(*f);
            String data = fis->readEntireStreamAsString();
            updateProject(*f);
            delete fis;
            delete f;
            setRunning(false);
            cleanUp();
            newFile();
            loadFromString(data);
            Project::getInstance()->setNewFile(false);
            setRunning(true);
        }

    }
    
}

ApplicationCommandTarget* SynthEditor::getNextCommandTarget() {
    return nullptr;
}
void SynthEditor::getAllCommands (Array<CommandID>& commands) {
    commands.add({ SynthEditor::CommandIds::NEW });
    // commands.add({ SynthEditor::CommandIds::ADD_MODULE });
    // commands.add({ SynthEditor::CommandIds::DUPLICATE });
    commands.add({ SynthEditor::CommandIds::DELETE_SELECTED });
    commands.add({ SynthEditor::CommandIds::SAVE });
    commands.add({ SynthEditor::CommandIds::LOAD });
    commands.add({ SynthEditor::CommandIds::LOAD_MODULE });
    commands.add({ SynthEditor::CommandIds::SAVE_MODULE });
    commands.add({ SynthEditor::CommandIds::SAVE_SCREENSHOT });
    commands.add({ SynthEditor::CommandIds::ALIGN_X });
    commands.add({ SynthEditor::CommandIds::ALIGN_Y });
    commands.add({ SynthEditor::CommandIds::RESET_GUI_POS });
}

void SynthEditor::getCommandInfo (CommandID commandID, ApplicationCommandInfo& result) {
    switch (commandID)
    {
        case SynthEditor::CommandIds::NEW:
            result.setInfo("New", String(), String(), 0);
            result.addDefaultKeypress('N', ModifierKeys::commandModifier);
            break;
        case SynthEditor::CommandIds::ADD_MODULE:
            result.setInfo("Add module", String(), String(), 0);
            break;
        case SynthEditor::CommandIds::DELETE_SELECTED:
            result.setInfo("Delete selected", String(), String(), 0);
            result.addDefaultKeypress(KeyPress::backspaceKey, ModifierKeys::commandModifier);
            break;
        case SynthEditor::CommandIds::SAVE:
            result.setInfo("Save", String(), String(), 0);
            result.addDefaultKeypress('s', ModifierKeys::commandModifier);
            break;
        case SynthEditor::CommandIds::LOAD:
            result.setInfo("Load", String(), String(), 0);
            result.addDefaultKeypress('l', ModifierKeys::commandModifier);
            break;
        case SynthEditor::CommandIds::LOAD_MODULE:
            result.setInfo("Load module", String(), String(), 0);
            break;
        case SynthEditor::CommandIds::SAVE_MODULE:
            result.setInfo("Save module", String(), String(), 0);
            break;
        case SynthEditor::CommandIds::SAVE_SCREENSHOT:
            result.setInfo("Save image", String(), String(), 0);
            break;
        case SynthEditor::CommandIds::DUPLICATE:
            result.setInfo("Duplicate module", String(), String(), 0);
            result.addDefaultKeypress('d', ModifierKeys::commandModifier);
            result.setActive(true);
            break;
        case SynthEditor::CommandIds::ALIGN_Y:
            result.setInfo("Align vertically", String(), String(), 0);
            break;
        case SynthEditor::CommandIds::ALIGN_X:
            result.setInfo("Align horizontally", String(), String(), 0);
            break;
        case SynthEditor::CommandIds::RESET_GUI_POS:
            result.setInfo("Reset UI position", String(), String(), 0);
            break;
    }
};

bool SynthEditor::perform (const InvocationInfo& info) {
    
    if (info.commandID == SynthEditor::CommandIds::NEW) {
        setRunning(false);
        cleanUp();
        newFile();
        updateProject(File());
        Project::getInstance()->setNewFile(true);
        setRunning(true);
        return true;
    }
    else if(info.commandID == SynthEditor::CommandIds::ADD_MODULE) {
        /*
        Module* m3 = new Module("Module");
        
        m3->setTopLeftPosition(mouseX, mouseY);
        m3->setIndex(Time::currentTimeMillis());
        
        addAndMakeVisible(m3);
        root->getModules()->push_back(m3);
        return true;
         */
        return false;
    }
    else if (info.commandID == SynthEditor::CommandIds::SAVE) {
        saveFile();
        return true;
    }
    else if(info.commandID == SynthEditor::CommandIds::LOAD) {
        setRunning(false);
        openFile();
        setRunning(true);
        return true;
    }
    /*
    else if(info.commandID == SynthEditor::CommandIds::LOAD_MODULE) {
        setRunning(false);
        Module* m = loadModule();
        setRunning(true);
        if (m != nullptr)
            return true;
    }
     */
    else if(info.commandID == SynthEditor::CommandIds::SAVE_MODULE) {
        saveModule(selectionModel.getSelectedModule());
        return true;
    }
    else if (info.commandID == SynthEditor::CommandIds::SAVE_SCREENSHOT) {
        saveScreenShot();
        return true;
    }
    /*
    else if (info.commandID == SynthEditor::CommandIds::DUPLICATE) {
        duplicateSelected();
        return true;
    }
     */
    else if(info.commandID == SynthEditor::CommandIds::DELETE_SELECTED) {
        RemoveSelectedAction* rsa = new RemoveSelectedAction(this);
        Project::getInstance()->getUndoManager()->beginNewTransaction();
        Project::getInstance()->getUndoManager()->perform(rsa);
        return true;
    }
    else if(info.commandID == SynthEditor::CommandIds::ALIGN_Y) {
        alignSelectedY();
        return true;
    }
    else if(info.commandID == SynthEditor::CommandIds::ALIGN_X) {
        alignSelectedX();
        return true;
    }
    else if(info.commandID == SynthEditor::CommandIds::RESET_GUI_POS) {
        resetGUIPosition();
        return true;
    }
    
    return true;
}

void SynthEditor::alignSelectedY() {
    
    if (selectionModel.getSelectedModules().size() >= 2) {
    
        int minX = getWidth();
        int minY = getHeight();
        
        if (currentLayer == Module::Layer::ALL) {
            
            // find the topmost-topleft coordinate
            
            for (int i = 0; i < selectionModel.getSelectedModules().size();i++) {
                if (selectionModel.getSelectedModules().at(i)->getPosition().getX() < minX) {
                    minX = selectionModel.getSelectedModules().at(i)->getPosition().getX();
                }
                if (selectionModel.getSelectedModules().at(i)->getPosition().getY() < minY) {
                    minY = selectionModel.getSelectedModules().at(i)->getPosition().getY();
                }
            }
            
            /// now position the modules in the desired raster
            for (int i = 0; i < selectionModel.getSelectedModules().size();i++) {
                selectionModel.getSelectedModules().at(i)->savePosition();
                selectionModel.getSelectedModules().at(i)->setTopLeftPosition(minX, minY + i *  selectionModel.getSelectedModules().at(i)->getHeight());
                
            }
        }
        else {
            // find the topmost-topleft coordinate of the UI positions
            
            for (int i = 0; i < selectionModel.getSelectedModules().size();i++) {
                if (selectionModel.getSelectedModules().at(i)->getUiPosition().getX() < minX) {
                    minX = selectionModel.getSelectedModules().at(i)->getUiPosition().getX();
                }
                if (selectionModel.getSelectedModules().at(i)->getPosition().getY() < minY) {
                    minY = selectionModel.getSelectedModules().at(i)->getUiPosition().getY();
                }
            }
            
            /// now position the modules in the desired raster
            for (int i = 0; i < selectionModel.getSelectedModules().size();i++) {
                selectionModel.getSelectedModules().at(i)->setUiPosition(minX, minY + i *  selectionModel.getSelectedModules().at(i)->getHeight());
                selectionModel.getSelectedModules().at(i)->saveUiPosition();
                selectionModel.getSelectedModules().at(i)->setTopLeftPosition(selectionModel.getSelectedModules().at(i)->getUiPosition());
            }
        }

    }

}

void SynthEditor::alignSelectedX() {
    if (selectionModel.getSelectedModules().size() >= 2) {
        
        
        int minX = getWidth();
        int minY = getHeight();
        
        if (currentLayer == Module::Layer::ALL) {
            
            // find the topmost-topleft coordinate for the GUI layer position
            
            for (int i = 0; i < selectionModel.getSelectedModules().size();i++) {
                if (selectionModel.getSelectedModules().at(i)->getPosition().getX() < minX) {
                    minX = selectionModel.getSelectedModules().at(i)->getPosition().getX();
                }
                if (selectionModel.getSelectedModules().at(i)->getPosition().getY() < minY) {
                    minY = selectionModel.getSelectedModules().at(i)->getPosition().getY();
                }
            }
            
            for (int i = 0; i < selectionModel.getSelectedModules().size();i++) {
                selectionModel.getSelectedModules().at(i)->savePosition();
                selectionModel.getSelectedModules().at(i)->setTopLeftPosition(minX + i * selectionModel.getSelectedModules().at(i)->getWidth(), minY);
            }
        }
        else {
            
            // find the topmost-topleft coordinate for the GUI layer position
            
            for (int i = 0; i < selectionModel.getSelectedModules().size();i++) {
                if (selectionModel.getSelectedModules().at(i)->getUiPosition().getX() < minX) {
                    minX = selectionModel.getSelectedModules().at(i)->getUiPosition().getX();
                }
                if (selectionModel.getSelectedModules().at(i)->getUiPosition().getY() < minY) {
                    minY = selectionModel.getSelectedModules().at(i)->getUiPosition().getY();
                }
            }
            
            for (int i = 0; i < selectionModel.getSelectedModules().size();i++) {
                selectionModel.getSelectedModules().at(i)->setUiPosition(minX + i * selectionModel.getSelectedModules().at(i)->getWidth(), minY);
                selectionModel.getSelectedModules().at(i)->saveUiPosition();
                selectionModel.getSelectedModules().at(i)->setTopLeftPosition(selectionModel.getSelectedModules().at(i)->getUiPosition());
            }
        }
    }
}

void SynthEditor::resetGUIPosition() {
     for (int i = 0; i < selectionModel.getSelectedModules().size();i++) {
         selectionModel.getSelectedModules().at(i)->resetUIPosition();
     }
}

void SynthEditor::updateProject(File file) {
    
    Project::getInstance()->setCurrentFilePath(file.getFullPathName());
    Project::getInstance()->setCurrentFileName(file.getFileName());
    
    for (int i = 0; i < tab->getNumTabs();i++) {
        
        Viewport* view = dynamic_cast<Viewport*>(tab->getTabContentComponent(i));
        
        if (view != nullptr) {
            
            SynthEditor* editor = dynamic_cast<SynthEditor*>(view->getViewedComponent());
            
            if (editor != nullptr && editor == this) {
                if (!file.exists()) {
                    tab->setTabName(i, "untitled");
                }
                else {
                    tab->setTabName(i, Project::getInstance()->getCurrentFileName());
                }
                
                break;
            }
        }
    }
    
    for (int i = 0;i < listener.size();i++) {
        listener.at(i)->fileChanged(file.getFullPathName());
    }
}
