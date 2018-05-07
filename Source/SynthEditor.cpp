
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

class SampleEditor;

String SynthEditor::defaultMidiOutputName = "Express 128 Port 7";
String SynthEditor::defaultMidiInputName = "Express 128 Port 7";


#define USE_PUSH

SynthEditor::SynthEditor(){
    
    setSize (1280, 800);

    root = new Module("Root");
    
    selectionModel = SelectionModel();
    
    setRepaintsOnMouseActivity(true);
    // setMouseClickGrabsKeyboardFocus(true);
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

    selectionModel.setRoot(root);
    
	setRepaintsOnMouseActivity(true);
	setMouseClickGrabsKeyboardFocus(true);
	setWantsKeyboardFocus(true);

    addChildComponent(root);
    
    linePath = Path();

}

SynthEditor::~SynthEditor()
{
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

    selectionModel.checkForConnection(e.getPosition());
}

void SynthEditor::mouseDown (const MouseEvent& e)
{
    if (locked) {
        return;
    }

    mouseDownX = e.getPosition().getX();
    mouseDownY = e.getPosition().getY();

	if (e.mods.isLeftButtonDown()) {
		isLeftMouseDown = true;
    }
    
    if (!isLeftShiftDown) {
        selectionModel.clearSelection();
    }

    selectionModel.select(e.getPosition());
    selectionModel.deselectAllPins();
    selectionModel.checkForConnection(e.getPosition());
    
    // has HitModule ?

    state = selectionModel.checkForHitAndSelect(e.getPosition());
    
    if (state == SelectionModel::State::NONE) {
        selectionModel.clearSelection();
        state = SelectionModel::State::DRAGGING_SELECTION;
    }
    else {
        selectionModel.checkForPinSelection(e.getPosition());
        notifyListeners();
    }
    
    // has any connection been clicked?
    
    selectionModel.checkForConnection(e.getPosition());
	
	if (e.mods.isRightButtonDown()) {
        showContextMenu(e.getPosition());
    }

    repaint();

    sendChangeMessage();
    notifyListeners();
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
                m->addItem(6, "Save module");
            }
            else {
                if ((k = dynamic_cast<Knob*>(module)) != NULL) {
                    m->addItem(3, "MIDI learn");
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
            else if (result == 6) {
                saveModule(selectionModel.getSelectedModule());
            }
        }
        else {
            m->addItem(1, "Add control");
            m->addItem(2, "Add constant");
            
            const int result = m->show();
            
            if (result == 0)
            {
                // user dismissed the menu without picking anything
            }
            else if (result == 1) {
                Knob* k = dynamic_cast<Knob*>(PrefabFactory::getInstance()->getPrefab(69, _sampleRate, bufferSize));
                addChangeListener(k);
                k->setValue(1);
                k->setName(module->getPins().at(pinIndex)->getName());
                Point<int> pos = module->getPinPosition(pinIndex);
                k->setTopLeftPosition(pos.getX() - 150, mouseY - k->getHeight() / 2);
                module->pins.at(pinIndex)->getConnections().push_back(k->getPins().at(0));
                addAndMakeVisible(k);
                Connection* con = new Connection();
                con->a = k->getPins().at(0);
                con->b = module->pins.at(pinIndex);
                con->source = k;
                con->target = module;
                root->getConnections()->push_back(con);
                root->getModules()->push_back(k);
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
                
                selectionModel.getSelectedModules().push_back(k);
                repaint();
                resized();
            }
            else if (result == 2) {
                Constant* c = dynamic_cast<Constant*>(PrefabFactory::getInstance()->getPrefab(67, _sampleRate, bufferSize));
                addChangeListener(c);
                c->setValue(1);
                c->setName(module->getPins().at(pinIndex)->getName());
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
            
        }
        
    }
    else {
        
        m->addItem(1, "Add module");
        m->addItem(2, "Save");
        m->addItem(3, "Load");
        m->addItem(4, "New");
        m->addItem(5, "Load module");
        m->addItem(6, "Save module");
        m->addItem(7, "Save picture");
        
        if (locked) {
            m->addItem(99, "Unlock");
            for (int i = 0; i < root->getModules()->size(); i++) {
                root->getModules()->at(i)->setInterceptsMouseClicks(false, true);
            }
        }
        else {
            for (int i = 0; i < root->getModules()->size(); i++) {
                // root->getModules()->at(i)->setInterceptsMouseClicks(true, true);
            }
             m->addItem(99, "Lock");
        }
        
        PopupMenu* prefabMenu = new PopupMenu();
        
        StringArray* categories = PrefabFactory::getInstance()->getCategories();
        
        for (int i = 0; i < categories->size();i++) {
            PopupMenu category = PopupMenu();
            
            std::map<int,PrefabFactory::Prefab> prefabs = PrefabFactory::getInstance()->getPrefabNames();
            
            for (std::map<int,PrefabFactory::Prefab>::iterator it  = prefabs.begin();it != prefabs.end();++it) {
                if ((*it).second.getCategory() == categories->getReference(i)) {
                    if ((*it).second.getIcon() != nullptr) {
                        // int itemResultID, const String& itemText, bool isActive, bool isTicked, const Image& iconToUs
                        category.addItem((*it).first,(*it).second.getName(), true,false,*(*it).second.getIcon());
                    }
                    else {
                        category.addItem((*it).first,(*it).second.getName(), true);
                    }
                 
                }
                
            }
            
            prefabMenu->addSubMenu(categories->getReference(i), category, true);
        }
        
        m->addSubMenu("Prefabs",*prefabMenu);
        
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
        else if (result == 1)
        {
            Module* m3 = new Module("Macro");
            
            m3->setTopLeftPosition(position.getX(), position.getY());
            m3->setIndex(Time::currentTimeMillis());
            
            addAndMakeVisible(m3);
            root->getModules()->push_back(m3);
            
        }
        else if (result == 2)
        {
            saveFile();
        }
        else if (result == 3) {
            setRunning(false);
            openFile();
            setRunning(true);
        }
        else if (result == 4) {
            setRunning(false);
            cleanUp();
            mixer->removeAllChannels();
            newFile();
            setRunning(true);
        }
        else if (result == 5) {
            running = false;
            Module* m = loadModule();
            root->getModules()->push_back(m);
            running = true;
        }
        else if (result == 6) {
            saveModule(selectionModel.getSelectedModule());
        }
        else if (result == 7) {
            saveScreenShot();
        }
        else if (result == 99) {
            locked = !locked;
        }
        else if (result >= 1000) {
            StringArray recent = Project::getInstance()->getRecentFiles();
            String path = recent.getReference(result - 1000);
            FileInputStream *fis = new FileInputStream(File(path));
            String data = fis->readEntireStreamAsString();
            delete fis;
            setRunning(false);
            cleanUp();
            mixer->removeAllChannels();
            newFile();
            loadFromString(data);
            setRunning(true);
        }
        else {
            AddModuleAction* am = new AddModuleAction(this,position,result);
            Project::getInstance()->getUndoManager()->beginNewTransaction();
            Project::getInstance()->getUndoManager()->perform(am);
        }
        
        delete prefabMenu;
    }
    
    delete m;

}

void SynthEditor::itemDropped (const SourceDetails& dragSourceDetails)  {
    AddModuleAction* am = new AddModuleAction(this,dragSourceDetails.localPosition,dragSourceDetails.description.toString().getIntValue());
    Project::getInstance()->getUndoManager()->beginNewTransaction();
    Project::getInstance()->getUndoManager()->perform(am);
};

void SynthEditor::mouseDrag (const MouseEvent& e)
{
    
    if (locked) {
        return;
    }
    
    mouseX = e.getPosition().getX();
    mouseY = e.getPosition().getY();
    
    dragDistanceX = e.getDistanceFromDragStartX();
    dragDistanceY = e.getDistanceFromDragStartY();
    
    dragHasStarted = true;
    
    if (state != SelectionModel::State::DRAGGING_SELECTION) {
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
    
	lineStopX = lineStartX + e.getDistanceFromDragStartX();
	lineStopY = lineStartY + e.getDistanceFromDragStartY();

    if (state == SelectionModel::State::MOVING_SELECTION) {
        for (int i = 0; i < root->getModules()->size(); i++) {
            
            Module* m = root->getModules()->at(i);
            
            if (m->isSelected()) {
                
                if (m->getSelectedPin() == nullptr || m->getSelectedPin() == NULL ) {
                    
                   
                    m->setTopLeftPosition(m->getSavedPosition().getX() + e.getOffsetFromDragStart().getX(), m->getSavedPosition().getY()+ e.getOffsetFromDragStart().getY());
                    

                    repaint();
                }
            }
            selectionModel.checkForPinSelection(e.getPosition());
        }
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
                    m->savePosition();
                    
                    selectionModel.getSelectedModules().push_back(m);
                }

            }
        }
    }

    
	repaint();
    resized();
}

void SynthEditor::mouseUp (const MouseEvent& e)
{
	if (e.mods.isLeftButtonDown()) {
		isLeftMouseDown = false;
        
        if (!dragHasStarted)
            selectionModel.clearSelection();
        
	}

    for (int i = 0; i < root->getModules()->size(); i++) {

        Module* m = root->getModules()->at(i);
        m->savePosition();
        
        if (state == SelectionModel::State::DRAGGING_CONNECTION)  {
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
    
    if (selectionModel.getSelectedModules().size() == 0) {
        showContextMenu(e.getPosition());
    }

    //[/UserCode_mouseDoubleClick]
}

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
    // startTimer(100);
}

void SynthEditor::timerCallback(){
    repaint();
}

void SynthEditor::setCurrentLayer(int layer) {
    
    currentLayer = static_cast<Module::Layer>(layer);
    
    for (int i = 0;i < root->getModules()->size();i++) {
        if (currentLayer == Module::Layer::ALL || root->getModules()->at(i)->getLayer() == currentLayer) {
            root->getModules()->at(i)->setVisible(true);
        }
        else {
            root->getModules()->at(i)->setVisible(false);
        }
    }
    
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
        removeSelectedItem();
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
    for(std::vector<Module*>::iterator it = root->getModules()->begin();it != root->getModules()->end();it++) {
        removeModule((*it));
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
    outputChannels.clear();
    inputChannels.clear();
    auxChannels.clear();
    Project::getInstance()->getUndoManager()->clearUndoHistory();
    removeAllChangeListeners();
    if (tab != nullptr) {
        
        int i = tab->getNumTabs();
        
        while(tab->getNumTabs() > 1) {
            tab->removeTab(--i);
        }
    }
    tab->setCurrentTabIndex(0);
    
    MainTabbedComponent* supplemental = Project::getInstance()->getSupplemental();
    
    if (supplemental != nullptr) {
        
        int i = supplemental->getNumTabs();
        
        while(supplemental->getNumTabs() > 1) {
            supplemental->removeTab(--i);
        }
    }
    supplemental->setCurrentTabIndex(0);
}


void SynthEditor::removeSelectedItem() {
    running = false;
    std::vector<Connection*>* cons = root->getConnections();
    cons->erase(std::remove_if(cons->begin(), cons->end(), [](Connection* c){
        if (c->selected){
            delete c;
            return true;
        }
        return false;
    }),cons->end());
    
    for(std::vector<Module*>::iterator it = root->getModules()->begin();it != root->getModules()->end();it++) {
        if ((*it)->isSelected())
            removeModule((*it));
    }
    for(std::vector<Module*>::iterator it = root->getModules()->begin();it != root->getModules()->end();) {
        if ((*it)->isSelected()) {
            delete *it;
            it = root->getModules()->erase(it);
        }
        else {
            ++it;
        }
    }
    selectionModel.clearSelection();
    running = true;
}

void SynthEditor::newFile() {
    root = new Module("Root");
    selectionModel = SelectionModel();
    selectionModel.setRoot(root);
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
    }
    
    xml = nullptr;
    resized();
    setRunning(true);
}

void SynthEditor::saveModule(Module* m) {

    FileChooser chooser("Select target file...", File::nonexistent, "*.slb");
    
    if (chooser.browseForFileToSave(true)) {
        
        ValueTree* v = new ValueTree("Module");
        
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
    FileChooser chooser("Select module to open", File::nonexistent, "*.slb");
    
	Module* m = nullptr;
    if (chooser.browseForFileToOpen()) {
        
        File file = chooser.getResult();
        ScopedPointer<XmlElement> xml = XmlDocument(file).getDocumentElement();
        
        Project::getInstance()->addRecentFile(file.getFullPathName());
        ValueTree v = ValueTree::fromXml(*xml.get());
        
        setRunning(false);
        Module* m = new Module(v.getChildWithName("Module").getProperty("name").toString());
        ModuleUtils::loadStructure(m->getModules(),m->getConnections(), &v, this);
        
        for (std::vector<Module*>::iterator it = m->getModules()->begin(); it != m->getModules()->end(); ++it) {
            addAndMakeVisible((*it));
        }
        
        resized();
        
        xml = nullptr;
 
    }
	return m;
}


void SynthEditor::saveFile() {
    
#if JUCE_IOS
    File path = File::getSpecialLocation(File::userApplicationDataDirectory);
    File outputDir = File(path.getFullPathName()+"/structure.xml");
    
    FileChooser chooser("Select target file...",outputDir, "*.slb");
    
    if (chooser.browseForFileToSave(true)) {
        
        ValueTree* v = new ValueTree("Synth");
        
        ModuleUtils::saveStructure(root->getModules(),root->getConnections(), v);
        
        URL file = chooser.getURLResult();
        
        OutputStream* os = file.createOutputStream();
        
        XmlElement* xml = v->createXml()
        
        xml->writeToStream(*os, "");
        
        delete os;
        delete xml;
        delete v;
    }
#else
    FileChooser chooser("Select target file...", File::nonexistent, "*.slb");
    
    if (chooser.browseForFileToSave(true)) {
        
        ValueTree* v = new ValueTree("Synth");
        
        ModuleUtils::saveStructure(root->getModules(),root->getConnections(), v);
        
        File file = chooser.getResult();
        
        XmlElement* xml = v->createXml();
        xml->writeToFile(file, "");

        delete xml;
        delete v;
        
    }
#endif
}

void SynthEditor::openEditor(Module *m) {
    
    SynthEditor* editor = new SynthEditor(_sampleRate, bufferSize);
    Viewport* editorView = new Viewport();
    editorView->setSize(500,200);
    editorView->setViewedComponent(editor);
    editorView->setScrollBarsShown(true,true);
    editorView->setScrollOnDragEnabled(false);
    editorView->setWantsKeyboardFocus(false);
    editorView->setMouseClickGrabsKeyboardFocus(false);
    editor->setModule(m, false);
    editor->setTab(tab);
    editor->addEditorListener(Project::getInstance()->getPropertyView());
    tab->addTab(m->getName(), Colours::darkgrey, editorView, false);
    openViews.push_back(editorView);
    tab->setCurrentTabIndex(tab->getNumTabs() - 1);
    editor->resized();
}

void SynthEditor::openSampleEditor(SamplerModule *sm) {
    Project::getInstance()->getSupplemental()->addTab("Sample editor", Colours::darkgrey, sm->getEditor(), true);
    Project::getInstance()->getSupplemental()->setCurrentTabIndex(Project::getInstance()->getSupplemental()->getNumTabs() - 1);
#ifdef USE_PUSH
    sm->updatePush2Pads();
    sm->updatePush2Display();
#endif
}

void SynthEditor::openStepSequencer(StepSequencerModule *ssm) {
    SequenceEditor* se = ssm->getEditor();
    Project::getInstance()->getSupplemental()->addTab("Step sequencer", Colours::darkgrey, se, true);
    Project::getInstance()->getSupplemental()->setCurrentTabIndex(Project::getInstance()->getSupplemental()->getNumTabs() - 1);
}

void SynthEditor::openRecorder(AudioRecorderModule *arm) {
    AudioRecorderEditor* are = arm->getEditor();
    Project::getInstance()->getSupplemental()->addTab("Audio recorder", Colours::darkgrey, are, true);
    Project::getInstance()->getSupplemental()->setCurrentTabIndex(Project::getInstance()->getSupplemental()->getNumTabs() - 1);
}

void SynthEditor::openFile() {
    FileChooser chooser("Select file to open", File::nonexistent, "*.slb");

    if (chooser.browseForFileToOpen()) {
            
        cleanUp();
        mixer->removeAllChannels();
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
        }

        resized();
        
        xml = nullptr;
        setRunning(true);
    }
}



void SynthEditor::configureAudioModule(Module *m, ChangeBroadcaster* broadcaster) {
    AudioManager* am = AudioManager::getInstance();
    
    AudioOut* out;
    
    if ((out = dynamic_cast<AudioOut*>(m)) != NULL) {
        outputChannels.push_back(out);
        String channelName = am->getOutputChannelNames().getReference(static_cast<int>(getOutputChannels().size()) - 1);
        int channelIndex = addChannel(channelName, Mixer::Channel::Type::OUT);
        out->setChannelIndex(channelIndex);
    }
    
    AudioIn* in;
    
    if ((in = dynamic_cast<AudioIn*>(m)) != NULL) {
        inputChannels.push_back(in);
        String channelName = am->getInputChannelNames().getReference(static_cast<int>(getInputChannels().size()) - 1);
        int channelIndex = addChannel(channelName, Mixer::Channel::Type::IN);
        in->setChannelIndex(channelIndex);
    }
    
    AuxOut* aux;
    
    if ((aux = dynamic_cast<AuxOut*>(m)) != NULL) {
        auxChannels.push_back(aux);
        String channelName = "Aux "+ String(getAuxChannels().size());
        int channelIndex = addChannel(channelName, Mixer::Channel::Type::AUX);
        aux->setChannelIndex(channelIndex);
    }
    
    broadcaster->addChangeListener(m);
    

}

void SynthEditor::setTab(juce::TabbedComponent *t) {
    this->tab = t;
}

void SynthEditor::removeModule(Module* module) {

    
    AudioOut* out = nullptr;
    
    if ((out = dynamic_cast<AudioOut*>(module)) != nullptr){
        mixer->removeChannel(out->getChannelIndex());
        for(std::vector<AudioOut*>::iterator it = outputChannels.begin();it != outputChannels.end();) {
            if ((*it)->getChannelIndex() == out->getChannelIndex()) {
                it = outputChannels.erase(it);
            }
            else {
                ++it;
            }
        }
    }

    
    AudioIn* in = nullptr;
    
    if ((in = dynamic_cast<AudioIn*>(module)) != nullptr){
        mixer->removeChannel(in->getChannelIndex());
        for(std::vector<AudioIn*>::iterator it = inputChannels.begin();it != inputChannels.end();) {
            if ((*it)->getChannelIndex() == in->getChannelIndex()) {
                it = inputChannels.erase(it);
            }
            else {
                ++it;
            }
        }
    }
    
    AuxOut* aux = nullptr;
    
    if ((aux = dynamic_cast<AuxOut*>(module)) != nullptr){
        mixer->removeChannel(aux->getChannelIndex());
        for(std::vector<AuxOut*>::iterator it = auxChannels.begin();it != auxChannels.end();) {
            if ((*it)->getChannelIndex() == aux->getChannelIndex()) {
                it = auxChannels.erase(it);
            }
            else {
                ++it;
            }
        }
    }
    
    vector<long> pinsToBeRemoved;
    
    removeChangeListener(module);
    
    // find the indices of the pins being involved in the disconnect

    for (int j = 0; j < root->getModules()->size(); j++) {
        if (root->getModules()->at(j)->getIndex() != module->getIndex()) {
            
            for (int k = 0; k < root->getModules()->at(j)->getPins().size(); k++) {
                
                // for every connection of each pin
                for (int l = 0; l < root->getModules()->at(j)->getPins().at(k)->getConnections().size(); l++) {
                    
                    // for each pin of the module being removed
                    for (int n = 0; n < module->getPins().size();n++) {
                        // if the index matches remove pin from vector
                        
                        if (root->getModules()->at(j)->getPins().at(k)->getConnections().at(l)->index == module->getPins().at(n)->index) {
                            pinsToBeRemoved.push_back(root->getModules()->at(j)->getPins().at(k)->getConnections().at(l)->index);
                            
                        }
                    }
                    
                }
            }
        }
    }
    
    // remove according pins
    
    for (int i = 0; i < pinsToBeRemoved.size();i++) {
        for (int j = 0; j < root->getModules()->size(); j++) {
            
            for (int k = 0; k < root->getModules()->at(j)->getPins().size();k++) {
                
                Pin* p = root->getModules()->at(j)->getPins().at(k);
                for (std::vector<Pin*>::iterator it = p->getConnections().begin();it != p->getConnections().end();) {
                    if ((*it)->index == pinsToBeRemoved.at(i)) {
                        it = p->getConnections().erase(it);
                    }
                    else {
                        ++it;
                    }
                }
            }
        }
    }
    
    // remove dangling connections from other modules
    
    std::vector<Connection*>* cons = root->getConnections();
   
    for(int i = 0;i < root->getModules()->size();i++) {
        for (std::vector<Connection*>::iterator it = cons->begin(); it != cons->end(); ) {
            if ((*it)->source->getIndex() == module->getIndex() ||
                (*it)->target->getIndex() == module->getIndex() ) {
                delete (*it);
                it = cons->erase(it);
            }
            else {
                ++it;
            }
        }
    }
    
    
}

void SynthEditor::deleteSelected(bool deleteAll) {
    
    if(!deleteAll) {
        for (int i = 0; i < selectionModel.getSelectedModules().size();i++) {
            
            removeModule(selectionModel.getSelectedModules().at(i));
        }
    }
    
    // handle connections at root level
    if (deleteAll) {
        std::vector<Connection*>* cons = root->getConnections();
        cons->erase(std::remove_if(cons->begin(), cons->end(), [](Connection* c){return true;}),cons->end());

    }
    else {
        std::vector<Connection*>* cons = root->getConnections();
        cons->erase(std::remove_if(cons->begin(), cons->end(), [](Connection* c){
            if (c->selected){
                return true;
            }
            return false;
        }),cons->end());
    }
    std::vector<Module*>* mods = root->getModules();
    
    if (deleteAll) {
        
        mods->erase(std::remove_if(mods->begin(), mods->end(), [](Module* m){
            delete m; return true;
        }),mods->end());
    }
    else {
        
        mods->erase(std::remove_if(mods->begin(), mods->end(), [](Module* m){
            bool selected =  m->isSelected();
            if (selected) {
                delete m;
                m = nullptr;
            }
            return selected;
        }),mods->end());
    }
    notifyListeners();
    sendChangeMessage();
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

bool SynthEditor::channelIsValid(int channel) {
    if (outputChannels.at(0)->getPins().at(channel)->getConnections().size() == 1 &&
        outputChannels.at(0)->getPins().at(channel)->getConnections().at(0) != nullptr &&
        outputChannels.at(0)->getPins().at(channel)->getConnections().at(0)->getAudioBuffer() != nullptr) {
        return true;
    }
    return false;
}

bool SynthEditor::auxChannelIsValid(int channel, int subchannel) {
    if (auxChannels.at(channel)->getPins().at(subchannel)->getConnections().size() == 1 &&
        auxChannels.at(channel)->getPins().at(subchannel)->getConnections().at(0) != nullptr &&
        auxChannels.at(channel)->getPins().at(subchannel)->getConnections().at(0)->getAudioBuffer() != nullptr) {
        return true;
    }
    return false;
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

std::vector<AudioIn*>& SynthEditor::getInputChannels() {
    return inputChannels;
}

std::vector<AudioOut*>& SynthEditor::getOutputChannels() {
    return outputChannels;
}

std::vector<AuxOut*>& SynthEditor::getAuxChannels() {
    return auxChannels;
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
    for (int i = 0;i < listener.size();i++) {
        listener.at(i)->selectionChanged(this->selectionModel.getSelectedModule());
    }
}

bool SynthEditor::isDirty() {
    return dirty;
}

void SynthEditor::setDirty(bool dirty) {
    this->dirty = dirty;
}

void SynthEditor::saveScreenShot() {
    Image image = createComponentSnapshot(getLocalBounds());
    JPEGImageFormat format;
    
    FileChooser chooser("Select target file...", File::nonexistent, "*");
    
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
        
        if (f->getFullPathName().endsWith("slb")) {
            FileInputStream *fis = new FileInputStream(*f);
            String data = fis->readEntireStreamAsString();
            delete fis;
            delete f;
            setRunning(false);
            cleanUp();
            mixer->removeAllChannels();
            newFile();
            loadFromString(data);
            setRunning(true);
        }

    }
    
}
