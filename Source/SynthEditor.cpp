
#include "SynthEditor.h"
#include "MidiGate.h"
#include "MidiNote.h"
#include "MidiOut.h"
#include "AudioOut.h"
#include "AudioIn.h"
#include "TrackIn.h"
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
#include <set>
#include <map>
#include <algorithm>
#include "Actions/AddModuleAction.h"
#include "Actions/AddConnectionAction.h"
#include "Actions/DuplicateModuleAction.h"
#include "Actions/RemoveSelectedAction.h"
#include "Actions/MoveSelectedAction.h"
#include "Project/Project.h"
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
#include "Plugins/PluginBrowserComponent.h"
#include "TrackUtil.h"
#include "GeometryViewComponent.h"
#include "GeometryModule.h"
#include "Actions/AlignVerticalAction.h"
#include "Actions/AlignHorizontalAction.h"

class SampleEditor;

SynthEditor::SynthEditor() {

	setSize(1280, 800);

	root = std::make_shared<Module>("Root");

	selectionModel = SelectionModel();

	setRepaintsOnMouseActivity(true);
	setMouseClickGrabsKeyboardFocus(true);
	setWantsKeyboardFocus(true);
	setInterceptsMouseClicks(true, true);	
	addChildComponent(root.get());
}

SynthEditor::SynthEditor(double sampleRate, int buffersize)
{
	Logger::writeToLog("Creating SynthEditor with sample rate " + String(sampleRate) + " kHz and buffer size of " + String(buffersize) + " bytes.");

	this->bufferSize = buffersize;
	this->_sampleRate = sampleRate;

	setSize(1280, 800);

	root = std::make_shared<Module>("Root");
	Project::getInstance()->setRoot(root.get());

	selectionModel.setRoot(root.get());

	setRepaintsOnMouseActivity(true);
	addChildComponent(root.get());

	linePath = Path();

	Project::getInstance()->getCommandManager()->registerAllCommandsForTarget(this);
	setInterceptsMouseClicks(true, true);
}

SynthEditor::~SynthEditor()
{
	setRunning(false);
	if (isRoot && deleteModuleWhenRemoved) {

		cleanUp();
		root = nullptr;
		if (tab != nullptr) {
			tab->clearTabs();
			delete tab;
		}

	}

	for (int i = 0; i < openViews.size(); i++) {
		delete openViews.at(i);
	}

	listener.clear();
	setRunning(true);
}

void SynthEditor::setLocked(bool lock) {
	this->locked = lock;
	setModuleLocked(this->getModule(), lock);
	selectionModel.clearSelection();
	repaint();
}

bool SynthEditor::isLocked() {
	return this->locked;
}

void SynthEditor::setModuleLocked(Module* m, bool locked) {

	if (m != nullptr) {

		m->setLocked(locked);

		for (int i = 0; i < m->getModules()->size(); i++) {
			if (m->getModules()->at(i) != nullptr) {
				m->getModules()->at(i)->setLocked(locked);
			}
		}
	}
}

void SynthEditor::paint(Graphics& g)
{
	g.fillAll(Colour(0xff222222));
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
	if (root == nullptr) {
		return;
	}
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

	setSize(getParentWidth() * 1.5, getParentHeight() * 1.5);

	if (root != nullptr && root->getConnections() != nullptr) {
		for (int i = 0; i < root->getConnections()->size(); i++) {
			Connection* c = root->getConnections()->at(i);

			if (c->source != NULL && c->target != NULL) {

				int x1 = c->source->getX() + c->a->x;
				int y1 = c->source->getY() + c->a->y + 5;
				int x2 = c->target->getX() + c->b->x;
				int y2 = c->target->getY() + c->b->y + 5;

				Point<int> p1 = Point<int>(x1, y1);
				Point<int> p2 = Point<int>(x2, y2);
				c->setPoints(p1, p2);
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
	linePath.startNewSubPath(p1.x, p1.y);

	linePath.cubicTo(p1.x, p1.y,
		p1.x + (p2.x - p1.x) * 0.3f, p1.y,
		p1.x + (p2.x - p1.x) * 0.5f, p1.y + (p2.y - p1.y) * 0.5f);

	linePath.cubicTo(p1.x + (p2.x - p1.x) * 0.5f, p1.y + (p2.y - p1.y) * 0.5f,
		p2.x - (p2.x - p1.x) * 0.3, p2.y,
		p2.x, p2.y);

	PathStrokeType wideStroke(2.0f);
	wideStroke.createStrokedPath(linePath, linePath);
}

void SynthEditor::mouseMove(const MouseEvent& e)
{
	mouseX = e.getPosition().getX();
	mouseY = e.getPosition().getY();

	selectionModel.checkForConnectionSelect(e.getPosition());
}

void SynthEditor::mouseDown(const MouseEvent& e)
{
	mouseDownX = e.getPosition().getX();
	mouseDownY = e.getPosition().getY();

	lineStartX = mouseDownX;
	lineStartY = mouseDownY;

	lineStopX = lineStartX;
	lineStopY = lineStartY;

	if (e.mods.isLeftButtonDown()) {
		isLeftMouseDown = true;

		selectionFrame.setBounds(mouseDownX, mouseDownY, 0, 0);

		selectionModel.deselectAllPins();
		selectionModel.checkForConnectionSelect(e.getPosition());

		// has Hit Module ?

		state = selectionModel.checkForHitAndSelect(e.getPosition(), isLeftShiftDown, true);

		if (state == SelectionModel::State::NONE) {
			state = SelectionModel::State::DRAGGING_SELECTION;
		}
		else {
			selectionModel.checkForPinSelection(e.getPosition());
			notifyListeners();
		}

		// has any connection been clicked?

		selectionModel.checkForConnectionSelect(e.getPosition());
	}
	else if (e.mods.isMiddleButtonDown()) {
		isMiddleMouseDown = true;
		cutConnectionAtPosition(e.getPosition());
	}
	else if (e.mods.isRightButtonDown()) {
		selectionModel.checkForHitAndSelect(e.getPosition(), isLeftShiftDown, false);
		selectionModel.checkForConnectionSelect(e.getPosition());
		selectionModel.checkForPinSelection(e.getPosition());
		notifyListeners();
		showCommonContextMenu(e.getPosition());
	}

	if (root->getModules()->size() > 0) {
		sendChangeMessage();
		notifyListeners();
	}
	repaint();

}

void SynthEditor::mouseDrag(const MouseEvent& e)
{
	dragHasStarted = true;

	mouseX = e.getPosition().getX();
	mouseY = e.getPosition().getY();

	dragDistanceX = e.getDistanceFromDragStartX();
	dragDistanceY = e.getDistanceFromDragStartY();

	lineStopX = lineStartX + e.getDistanceFromDragStartX();
	lineStopY = lineStartY + e.getDistanceFromDragStartY();

	// middle mouse = cut connections
	if (isMiddleMouseDown) {
		cutConnectionsAlongLine(Point<int>(mouseDownX, mouseDownY), e.getPosition());
		repaint();
		return;
	}

	if (state != SelectionModel::State::DRAGGING_SELECTION) {
		checkForConnectionDrag();
	}
	if (state == SelectionModel::State::MOVING_SELECTION && !locked) {
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

			if (m->getSelectedPin() == nullptr || m->getSelectedPin() == NULL) {

				if (currentLayer == Module::Layer::GUI) {

					int x = m->getSavedUiPosition().getX() + e.getOffsetFromDragStart().getX();
					int y = m->getSavedUiPosition().getY() + e.getOffsetFromDragStart().getY();

					if (snapToGrid) {
						x = UITools::snap(x, 10, 4);
						y = UITools::snap(y, 10, 4);
					}

					m->setUiPosition(x, y);
					m->setTopLeftPosition(x, y);
				}
				else {

					int x = m->getSavedPosition().getX() + e.getOffsetFromDragStart().getX();
					int y = m->getSavedPosition().getY() + e.getOffsetFromDragStart().getY();

					if (snapToGrid) {
						x = UITools::snap(x, 10, 4);
						y = UITools::snap(y, 10, 4);

					}

					m->setTopLeftPosition(x, y);

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
	selectionFrame.setBounds(x, y, abs(dragDistanceX), abs(dragDistanceY));

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

void SynthEditor::mouseUp(const MouseEvent& e)
{
	if (e.mods.isLeftButtonDown()) {
		isLeftMouseDown = false;
	}
	if (e.mods.isMiddleButtonDown()) {
		isMiddleMouseDown = false;
	}

	if (state == SelectionModel::MOVING_SELECTION) {
		MoveSelectedAction* msa = new MoveSelectedAction(this);
		Project::getInstance()->getUndoManager()->beginNewTransaction();
		Project::getInstance()->getUndoManager()->perform(msa);
	}

	else if (state == SelectionModel::State::DRAGGING_CONNECTION) {
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

void SynthEditor::mouseDoubleClick(const MouseEvent& e)
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

				SamplerModule* sm = dynamic_cast<SamplerModule*>(m);

				if (sm != NULL) {
					openSampleEditor(sm);
				}

				PluginModule* pm = dynamic_cast<PluginModule*>(m);

				if (pm != NULL) {
					pm->openPluginWindow();
				}

				StepSequencerModule* ssm = dynamic_cast<StepSequencerModule*>(m);

				if (ssm != NULL) {
					openStepSequencer(ssm);
				}

				AudioRecorderModule* arm = dynamic_cast<AudioRecorderModule*>(m);

				if (arm != NULL) {
					openRecorder(arm);
				}

				GeometryModule* gm = dynamic_cast<GeometryModule*>(m);

				if (gm != NULL) {

					class GeometryWindow : public juce::DialogWindow {

					public:

						GeometryWindow() : juce::DialogWindow("Geometry", juce::Colours::darkgrey, true) {
							setUsingNativeTitleBar(true);
							setResizable(true, true);
						}

						void closeButtonPressed() override {
							setVisible(false);
							exitModalState(0);
							delete this;
						}
					};

					std::unique_ptr<GeometryViewComponent> rcc = std::make_unique<GeometryViewComponent>(gm);
					rcc->setOpaque(false);
					rcc->startTimer(30);
					gm->addChangeListener(rcc.get());

					auto* window = new GeometryWindow();
					window->setContentOwned(rcc.release(), true);
					window->centreWithSize(800, 600);
					window->setVisible(true);
					window->setAlwaysOnTop(true);

					Project::getInstance()->getOpenWindows().push_back(window);
				}
			}
			else {
				if (m->isSelected() && m->isEditable()) {
					openEditor(m);
				}
			}
		}
	}
}

bool::SynthEditor::showPinContextMenu(Point<int> position, Module* module, int pinIndex) {

	PopupMenu menu = PopupMenu();

	menu.addItem(1, "Add control");
	menu.addItem(2, "Add constant");
	menu.addItem(3, "Add terminal");

	const int result = menu.show();

	if (result == 0)
	{
		// user dismissed the menu without picking anything
	}
	else if (result == 1) {
		addControl(module, pinIndex);
		menu.setLookAndFeel(nullptr);
		return true;
	}
	else if (result == 2) {
		addConstant(module, pinIndex);
		menu.setLookAndFeel(nullptr);
		return true;
	}
	else if (result == 3) {
		addTerminal(module, pinIndex);
		menu.setLookAndFeel(nullptr);
		return true;
	}
		
	menu.setLookAndFeel(nullptr);

	return false;
}

bool SynthEditor::showModuleContextMenu(Point<int> position, Module* module) {

	PopupMenu menu = PopupMenu();

	if (module->isEditable()) {
		menu.addItem(1, "Add input");
		menu.addItem(2, "Add output");
		menu.addItem(3, "Duplicate");
		menu.addItem(4, "Save module");
	}

	Knob* k = nullptr;

	if ((k = dynamic_cast<Knob*>(module)) != nullptr) {
		menu.addItem(5, "MIDI learn");		
	}

	if (menu.getNumItems() > 0) {
		const int result = menu.show();

		if (result == 0)
		{
			// user dismissed the menu without picking anything
		}
		else if (result == 1) {
			module->addPin(Pin::Direction::IN);
			menu.setLookAndFeel(nullptr);
			return true;
		}
		else if (result == 2) {
			module->addPin(Pin::Direction::OUT);
			menu.setLookAndFeel(nullptr);
			return true;
		}
		else if (result == 3) {
			duplicateSelected();
			menu.setLookAndFeel(nullptr);
			return true;
		}
		else if (result == 4) {
			saveModule(selectionModel.getSelectedModule());
			menu.setLookAndFeel(nullptr);
			return true;
		}
		else if (result == 5) {
			k->setLearning(true);
			menu.setLookAndFeel(nullptr);
			return true;
		}
	}

	menu.setLookAndFeel(nullptr);
}

void SynthEditor::showPrefabMenu(Point<int> position) {
	
	PopupMenu prefabMenu = PopupMenu();
	prefabMenu.setLookAndFeel(Project::getInstance()->getLookAndFeel());

	StringArray* categories = PrefabFactory::getInstance()->getCategories();

	for (int i = 0; i < categories->size(); i++) {
		PopupMenu category = PopupMenu();

		std::map<int, PrefabFactory::Prefab> prefabs = PrefabFactory::getInstance()->getPrefabNames();

		for (std::map<int, PrefabFactory::Prefab>::iterator it = prefabs.begin(); it != prefabs.end(); ++it) {
			if ((*it).second.getCategory() == categories->getReference(i)) {

				if (((*it).second.getName().equalsIgnoreCase("Audio In") || (*it).second.getName().equalsIgnoreCase("Audio Out")) && !isRoot) {
					continue;
				}

				if ((*it).second.getIcon() != nullptr) {
					// int itemResultID, const String& itemText, bool isActive, bool isTicked, const Image& iconToUs
					category.addItem((*it).first, (*it).second.getName(), true, false, *(*it).second.getIcon());
				}
				else {
					category.addItem((*it).first, (*it).second.getName(), true);
				}
			}
		}

		prefabMenu.addSubMenu(categories->getReference(i), category, true);
	}

	const int result = prefabMenu.show();
	prefabMenu.setLookAndFeel(nullptr);


	if (result > 0) {
		AddModuleAction* am = new AddModuleAction(this, position, result);
		Project::getInstance()->getUndoManager()->beginNewTransaction();
		Project::getInstance()->getUndoManager()->perform(am);
	}

}

void SynthEditor::showCommonContextMenu(Point<int> position) {

	PopupMenu commonMenu = PopupMenu();
	commonMenu.setLookAndFeel(Project::getInstance()->getLookAndFeel());	

	Module* module = nullptr;

	if (selectionModel.getSelectedModule() != nullptr) {
		module = selectionModel.getSelectedModule();
	}

	if (module != nullptr && selectionModel.getSelectedModules().size() == 1) {

		int pinIndex = -1;

		for (int i = 0; i < module->getPins().size(); i++) {
			if (module->getPins().at(i)->isSelected()) {
				pinIndex = i;
			}
		}

		// found pin 
		if (pinIndex >= 0) {
			if (showPinContextMenu(position, module, pinIndex)) {
				return;
			}
		}
		else {

			if (showModuleContextMenu(position, module)) {
				return;
			}
		}
	}

	// no module selected, or multiple modules selected
	PopupMenu menu = PopupMenu();

	// Add general items
	menu.addItem(1, "Add module");
	menu.addCommandItem(Project::getInstance()->getCommandManager(), CommandIds::DELETE_SELECTED);
	menu.addItem(3, "Load module");
	if (selectionModel.getSelectedModules().size() > 1)
		menu.addItem(98, "Create Module from Selection");
	menu.addCommandItem(Project::getInstance()->getCommandManager(), CommandIds::SAVE_MODULE);
	menu.addCommandItem(Project::getInstance()->getCommandManager(), CommandIds::SAVE_SCREENSHOT);

	if (locked) {
		menu.addItem(99, "Unlock");
		for (int i = 0; i < root->getModules()->size(); i++) {
			// root->getModules()->at(i)->setInterceptsMouseClicks(false, true);
		}
	}
	else {
		for (int i = 0; i < root->getModules()->size(); i++) {
			// root->getModules()->at(i)->setInterceptsMouseClicks(true, true);
		}
		menu.addItem(99, "Lock");
	}

	PopupMenu alignMenu = PopupMenu();

	alignMenu.addCommandItem(Project::getInstance()->getCommandManager(), CommandIds::ALIGN_X);
	alignMenu.addCommandItem(Project::getInstance()->getCommandManager(), CommandIds::ALIGN_Y);
	alignMenu.addCommandItem(Project::getInstance()->getCommandManager(), CommandIds::ALIGN_TO_GRID);

	menu.addSubMenu("Align", alignMenu);

	PopupMenu recentFiles = PopupMenu();

	Project::getInstance()->loadRecentFileList();
	StringArray recent = Project::getInstance()->getRecentFiles();

	for (int i = 0; i < recent.size(); i++) {
		recentFiles.addItem(i + 1000, recent.getReference(i));
	}

	menu.addSubMenu("Recent files", recentFiles);

	const int result = menu.show();

	if (result == 0)
	{
		// user dismissed the menu without picking anything
	}
	else if (result == 1) {
		Module* m = new Module("Module");
		m->setIndex(PrefabFactory::getInstance()->getNextModuleIndex());
		AddModuleAction* am = new AddModuleAction(this, position, m->getIndex());
		am->setModule(m);
		Project::getInstance()->getUndoManager()->beginNewTransaction();
		Project::getInstance()->getUndoManager()->perform(am);
	}
	else if (result == 3) {
		setRunning(false);
		loadModule();
		setRunning(true);
	}
	else if (result == 98) {
		createModuleFromSelection();
	}
	else if (result == 99) {
		setLocked(!locked);
	}
	else if (result >= 1000) {
		recent = Project::getInstance()->getRecentFiles();
		String path = recent.getReference(result - 1000);
		File file = File(path);

		if (file.exists()) {
			FileInputStream* fis = new FileInputStream(file);
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

	menu.setLookAndFeel(nullptr);	

}

void SynthEditor::connect(Module* root, Module* c, Module* module, int pinIndex, int mouseY) {

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
		
	k->setValue(1);
	k->setName(module->getPins().at(pinIndex)->getName());
	
	addChangeListener(k);
	connect(root.get(), k, module, pinIndex, mouseY);

	selectionModel.clearSelection();
	k->setSelected(true);

	if (module->getName().contains("Filter")) {
		k->setMinimum(100);
		k->setMaximum(15000);
		k->setStepSize(10);
	}
	if (module->getName().contains("Mixer")) {
		k->setMinimum(0);
		k->setMaximum(1);
		k->setStepSize(0.01);
	}
	if (module->getName().contains("ADSR")) {

		k->setMinimum(0);

		if (module->getPins().at(pinIndex)->getName() == "S") {
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

void SynthEditor::addConstant(Module* module, int pinIndex) {

	Constant* c = dynamic_cast<Constant*>(PrefabFactory::getInstance()->getPrefab(67, _sampleRate, bufferSize));
	
	addChangeListener(c);
	c->setValue(1);
	c->setName(module->getPins().at(pinIndex)->getName());
	
	connect(root.get(), c, module, pinIndex, mouseY);
}

void SynthEditor::addTerminal(Module* module, int pinIndex) {

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

	connect(root.get(), t, module, pinIndex, mouseY);
	addAndMakeVisible(t);
	getModule()->repaint();

}

void SynthEditor::createModuleFromSelection() {

    auto& sel = selectionModel.getSelectedModules();
    if (sel.size() < 2) return;

    // 1. Compute bounding box of selected modules
    int minX = sel[0]->getX(), maxX = sel[0]->getX() + sel[0]->getWidth();
    int minY = sel[0]->getY(), maxY = sel[0]->getY() + sel[0]->getHeight();
    for (auto* m : sel) {
        if (m->getX() < minX)                    minX = m->getX();
        if (m->getY() < minY)                    minY = m->getY();
        if (m->getX() + m->getWidth()  > maxX)   maxX = m->getX() + m->getWidth();
        if (m->getY() + m->getHeight() > maxY)   maxY = m->getY() + m->getHeight();
    }

    const int padding = 24;
    int groupX = minX - padding;
    int groupY = minY - padding;
    int groupW = (maxX - minX) + 2 * padding;
    int groupH = (maxY - minY) + 2 * padding;

    // 2. Create the container module
    Module* group = new Module("Group");
    group->setIndex(PrefabFactory::getInstance()->getNextModuleIndex());
    group->setSize(groupW, groupH);
    group->setTopLeftPosition(groupX, groupY);

    // Build a set of selected modules for fast lookup
    std::set<Module*> selSet(sel.begin(), sel.end());
    auto isInSel = [&selSet](Module* m) { return m != nullptr && selSet.count(m) > 0; };

    // 3. Move selected modules into the group
    //    Adjust their positions to be relative to the group interior
    for (auto* m : sel) {
        auto& rootMods = *root->getModules();
        rootMods.erase(std::remove(rootMods.begin(), rootMods.end(), m), rootMods.end());
        removeChildComponent(m);
        m->setTopLeftPosition(m->getX() - groupX, m->getY() - groupY);
        group->getModules()->push_back(m);
    }

    // 4. Classify and rewire connections crossing the selection boundary
    // Track source pins that already have an OUT terminal to avoid duplicates
    std::map<Pin*, Pin*> outTerminalForPin; // internal source pin → group extPin

    std::vector<Connection*> allRootConns = *root->getConnections();
    int inTermY  = padding;
    int outTermY = padding;

    for (auto* conn : allRootConns) {
        bool srcIn = isInSel(conn->source);
        bool tgtIn = isInSel(conn->target);

        if (srcIn && tgtIn) {
            // Pure internal: move connection to the group
            auto& rc = *root->getConnections();
            rc.erase(std::remove(rc.begin(), rc.end(), conn), rc.end());
            group->addConnection(conn);

        } else if (!srcIn && tgtIn) {
            // Crossing IN: external source feeds internal target
            Pin* tgtPin = conn->b;

            auto* term = dynamic_cast<TerminalModule*>(
                PrefabFactory::getInstance()->getPrefab(83, getSampleRate(), getBufferSize()));
            if (!term) continue;

            term->setType(tgtPin->getType());

            Pin* extPin = new Pin(tgtPin->getType());
            term->getPins().at(0)->setTerminal(extPin);
            term->getPins().at(0)->setType(tgtPin->getType());
            extPin->listeners.push_back(term);
            extPin->setName(term->getName());
            term->addChangeListener(extPin);

            group->addPin(Pin::Direction::IN, extPin);
            term->setIndex(extPin->index);

            // Position terminal on the left edge of the group interior
            term->setSize(80, 40);
            term->setTopLeftPosition(4, inTermY);
            inTermY += 44;

            // Internal connection: term.out → target.tgtPin
            auto* internalConn = new Connection(term, term->getPins().at(0), conn->target, tgtPin);
            tgtPin->getConnections().push_back(term->getPins().at(0));
            // Remove old source pin from tgtPin's connections list
            auto& tc = tgtPin->getConnections();
            tc.erase(std::remove(tc.begin(), tc.end(), conn->a), tc.end());

            group->getModules()->push_back(term);
            group->addConnection(internalConn);

            // Redirect external connection to the group
            extPin->getConnections().push_back(conn->a);
            conn->target = group;
            conn->b = extPin;

        } else if (srcIn && !tgtIn) {
            // Crossing OUT: internal source feeds external target
            Pin* srcPin = conn->a;

            Pin* extPin = nullptr;

            if (outTerminalForPin.count(srcPin) == 0) {
                // First time we see this source pin crossing out: create an OUT terminal
                auto* term = dynamic_cast<TerminalModule*>(
                    PrefabFactory::getInstance()->getPrefab(84, getSampleRate(), getBufferSize()));
                if (!term) continue;

                term->setType(srcPin->getType());

                extPin = new Pin(srcPin->getType());
                term->getPins().at(0)->setTerminal(extPin);
                term->getPins().at(0)->setType(srcPin->getType());
                extPin->listeners.push_back(term);
                extPin->setName(term->getName());
                term->addChangeListener(extPin);

                group->addPin(Pin::Direction::OUT, extPin);
                term->setIndex(extPin->index);

                // Position terminal on the right edge of the group interior
                term->setSize(80, 40);
                term->setTopLeftPosition(groupW - 84, outTermY);
                outTermY += 44;

                // Internal connection: source.srcPin → term.in
                auto* internalConn = new Connection(conn->source, srcPin, term, term->getPins().at(0));
                term->getPins().at(0)->getConnections().push_back(srcPin);

                group->getModules()->push_back(term);
                group->addConnection(internalConn);

                outTerminalForPin[srcPin] = extPin;
            } else {
                extPin = outTerminalForPin[srcPin];
            }

            // Redirect external connection to come from the group
            auto& tc = conn->b->getConnections();
            tc.erase(std::remove(tc.begin(), tc.end(), conn->a), tc.end());
            tc.push_back(extPin);

            conn->source = group;
            conn->a = extPin;
        }
        // else: neither endpoint in selection → leave unchanged
    }

    // 5. Add the group to the root and make it visible
    group->setResizable(true);
    root->getModules()->push_back(group);
    addAndMakeVisible(group);

    // 6. Select only the group
    selectionModel.clearSelection();
    group->setSelected(true);
    selectionModel.getSelectedModules().push_back(group);

    repaint();
    resized();
}

void SynthEditor::itemDropped(const SourceDetails& dragSourceDetails) {
	AddModuleAction* am = new AddModuleAction(this, dragSourceDetails.localPosition, dragSourceDetails.description.toString().getIntValue());
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
}

void SynthEditor::setRunning(bool running) {

	auto& bus = FastBus::getInstance();

	bus.publish("running", running);

	this->running = running;

	CriticalSection& c = AudioManager::getInstance()->getDeviceManager()->getAudioCallbackLock();

	if (!running) {

		c.enter();
	}
	else {
		c.exit();
	}
}

void SynthEditor::timerCallback() {
	repaint();
}

void SynthEditor::setCurrentLayer(int layer) {

	restoreRootLevelControls();

	currentLayer = static_cast<Module::Layer>(layer);

	selectionModel.setCurrentLayer(layer);

	for (int i = 0; i < root->getModules()->size(); i++) {
		root->getModules()->at(i)->setCurrentLayer(currentLayer);
		if (currentLayer == Module::Layer::ALL || root->getModules()->at(i)->getLayer() == currentLayer) {

			if (currentLayer == Module::Layer::GUI) {
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

	if (currentLayer == Module::Layer::GUI)
		collectRootLevelControls(root.get());

	repaint();
	resized();
}

void SynthEditor::collectRootLevelControls(Module* container) {
	for (auto* m : *container->getModules()) {
		if (m->getLayer() == Module::Layer::GUI && m->getDisplayAtRootLevel() && m->getParentComponent() != this) {
			RootLevelProxy proxy;
			proxy.module = m;
			proxy.originalParent = m->getParentComponent();
			proxy.originalPosition = m->getPosition();

			auto posInRoot = getLocalPoint(m->getParentComponent(), m->getPosition());
			m->setTopLeftPosition(posInRoot.x, posInRoot.y);
			addAndMakeVisible(m);

			rootLevelProxies.push_back(proxy);
		}
		if (!m->getModules()->empty())
			collectRootLevelControls(m);
	}
}

void SynthEditor::restoreRootLevelControls() {
	for (auto& proxy : rootLevelProxies) {
		if (proxy.originalParent != nullptr && proxy.module != nullptr) {
			proxy.module->setTopLeftPosition(proxy.originalPosition.x, proxy.originalPosition.y);
			proxy.originalParent->addAndMakeVisible(proxy.module);
		}
	}
	rootLevelProxies.clear();
}

int SynthEditor::getCurrentLayer() {
	return static_cast<int>(currentLayer);
}

Module::Layer SynthEditor::getCurrentLayerEnum()
{
	return Module::Layer();
}

SelectionModel& SynthEditor::getSelectionModel() {
	return selectionModel;
}

void SynthEditor::setMixer(std::shared_ptr<MixerPanel> mixer) {
	this->mixer = mixer;
}

std::shared_ptr<MixerPanel> SynthEditor::getMixer() {
	return mixer;
}

bool SynthEditor::keyPressed(const KeyPress& key, Component* originatingComponent)
{
	if (key.getKeyCode() == 65 && isLeftShiftDown) {
		showPrefabMenu(Point<int>(mouseX, mouseY));
	}
	else if (key.getKeyCode() == 65 && isCtrlDown) {
		for (int i = 0; i < root->getModules()->size(); i++) {
			root->getModules()->at(i)->setSelected(true);
			selectionModel.getSelectedModules().push_back(root->getModules()->at(i));
		}
	}

	repaint();
	return true;
}

bool SynthEditor::keyStateChanged(bool isKeyDown)
{
	return false;
}

void SynthEditor::modifierKeysChanged(const ModifierKeys& modifiers)
{
	isAltDown = modifiers.isAltDown();
	isCtrlDown = modifiers.isCtrlDown() || modifiers.isCommandDown();
	isLeftShiftDown = modifiers.isShiftDown();
}

void SynthEditor::cleanUp() {
	if (navigator != nullptr)
		navigator->clearTracks();

	mixer->removeAllChannels();
	for (std::vector<Module*>::iterator it = root->getModules()->begin(); it != root->getModules()->end(); it++) {
		ModuleUtils::removeModule(root.get(), (*it), getMixer().get(), this);
	}
	for (std::vector<Module*>::iterator it = root->getModules()->begin(); it != root->getModules()->end();) {
		for (std::vector<Connection*>::iterator it2 = (*it)->getConnections()->begin(); it2 != (*it)->getConnections()->end();) {
			delete* it2;
			it2 = (*it)->getConnections()->erase(it2);
		}
		delete* it;
		it = root->getModules()->erase(it);
	}
	for (std::vector<Connection*>::iterator it = root->getConnections()->begin(); it != root->getConnections()->end();) {
		delete* it;
		it = root->getConnections()->erase(it);
	}

	PrefabFactory::getInstance()->reset();

	root = nullptr;
	selectionModel.getSelectedModules().clear();
	mixer->clearChannels();
	Project::getInstance()->getUndoManager()->clearUndoHistory();
	removeAllChangeListeners();

	if (tab != nullptr) {

		int i = tab->getNumTabs();

		while (tab->getNumTabs() > 1) {
			tab->removeTab(--i);
		}
	}

	if (tab != nullptr)
		tab->setCurrentTabIndex(0);

	if (Project::getInstance()->getAppMode() == Project::AppMode::STUDIO) {
		MainTabbedComponent* supplemental = Project::getInstance()->getSupplemental();

		if (supplemental != nullptr) {

			int i = supplemental->getNumTabs();

			while (supplemental->getNumTabs() > 1) {
				supplemental->removeTab(--i);
			}
		}
		supplemental->setCurrentTabIndex(0);
	}
}

void SynthEditor::newFile() {

	setRunning(false);
	bool makeNewFile = true;
	
	if (isDirty()) {
		makeNewFile = juce::AlertWindow::showOkCancelBox(juce::AlertWindow::AlertIconType::NoIcon, "Hey!", "You have unsaved resources, wanna start from scratch anyway?");
	}
	
	if (makeNewFile) {
		Project::getInstance()->getUndoManager()->clearUndoHistory();
		root = std::make_shared<Module>("Root");
		Project::getInstance()->setRoot(root.get());
		selectionModel = SelectionModel();
		selectionModel.setRoot(root.get());
		setLocked(true);
		updateProject(File());
		setLocked(false);
		repaint();	
	}

	setRunning(true);

}

void SynthEditor::loadFromString(juce::String in) {

	std::unique_ptr<XmlElement> xml = XmlDocument(in).getDocumentElement();

	ValueTree v = ValueTree::fromXml(*xml.get());

	setRunning(false);
	ModuleUtils::loadStructure(root->getModules(), root->getConnections(), &v, this);

	{
		long maxIdx = 0;
		for (auto* m : *root->getModules())
			if (m->getIndex() > maxIdx) maxIdx = m->getIndex();
		PrefabFactory::getInstance()->updateLastModuleIndex(maxIdx);
	}

	for (std::vector<Module*>::iterator it = root->getModules()->begin(); it != root->getModules()->end(); ++it) {
		addAndMakeVisible((*it));
		configureAudioModule((*it), this);
		ImageModule* im;
		if ((im = dynamic_cast<ImageModule*>(*it)) != NULL) {
			im->toBack();
		}
	}

	if (mixer != nullptr)
		mixer->reconstructAllFXChains();

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
		ModuleUtils::saveStructure(m->getModules(), m->getConnections(), v);
		ModuleUtils::updateIndices(*v, Time::getMillisecondCounter());

		File file = chooser.getResult();

		std::unique_ptr<XmlElement> xml = v->createXml();
		xml->writeToFile(file, "");
		xml = nullptr;

		delete v;
	}
}

Module* SynthEditor::loadModule() {
	FileChooser chooser("Select module to open", File(), "*");

	if (chooser.browseForFileToOpen()) {

		File file = chooser.getResult();
		std::unique_ptr<XmlElement> xml = XmlDocument(file).getDocumentElement();

		Project::getInstance()->addRecentFile(file.getFullPathName());
		ValueTree v = ValueTree::fromXml(*xml.get());

		setRunning(false);

		Module* module = ModuleUtils::loadFromXml(v, this);
		module->setName(v.getProperty("name"));
		module->setTopLeftPosition(mouseX, mouseY);

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
	File outputDir = File(path.getFullPathName() + "/structure.xml");

	URL* file = nullptr;

	ValueTree* v = new ValueTree("Synth");

	ModuleUtils::saveStructure(root->getModules(), root->getConnections(), v);

	v->setProperty("lock", isLocked(), nullptr);
	v->setProperty("layer", static_cast<int>(currentLayer), nullptr);

	if (Project::getInstance()->isNewFile()) {

		FileChooser chooser("Select target file...", outputDir, "*");

		if (chooser.browseForFileToSave(true)) {
			URL url = chooser.getURLResult();
			file = &url;
		}

	}
	else {
		file = new URL(Project::getInstance()->getCurrentFilePath());
	}

	OutputStream* os = file->createOutputStream();
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
	v->setProperty("layer", static_cast<int>(currentLayer), nullptr);

	ModuleUtils::saveStructure(root->getModules(), root->getConnections(), v);

	if (Project::getInstance()->isNewFile()) {

		FileChooser chooser("Select target file...", File(), "*.slb");

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
		std::unique_ptr<XmlElement> xml = v->createXml();
		xml->writeToFile(file, "");
		xml = nullptr;
		delete v;

		if (navigator != nullptr) {
			ValueTree tracks = navigator->getConfiguration();
			xml = tracks.createXml();
			xml->writeToFile(file.getParentDirectory().getFullPathName() + "\\" + file.getFileNameWithoutExtension() + "_tracks.xml", "");
			xml = nullptr;
		}

		setDirty(false);
		Project::getInstance()->save(file.getParentDirectory().getFullPathName() + "\\" + file.getFileNameWithoutExtension() + "_audio.xml");
		Project::getInstance()->setNewFile(false);
		updateProject(file);
	}
#endif

}

void SynthEditor::openEditor(Module* m) {

	for (int i = 0; i < tab->getNumTabs(); i++) {

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
	editorView->setSize(500, 200);
	editorView->setViewedComponent(editor);
	editorView->setScrollBarsShown(true, true);
	editorView->setScrollOnDragEnabled(false);
	editorView->setWantsKeyboardFocus(false);
	editorView->setMouseClickGrabsKeyboardFocus(false);
	editor->setModuleRef(m);
	editor->setIndex(static_cast<int>(m->getIndex()));
	editor->setTab(tab);
	editor->addEditorListener(Project::getInstance()->getPropertyView());
	tab->addTab(m->getName(), Colours::darkgrey, editorView, false);
	openViews.push_back(editorView);
	tab->setCurrentTabIndex(tab->getNumTabs() - 1);
	editor->resized();
}

void SynthEditor::openSampleEditor(SamplerModule* sm) {

	MainTabbedComponent* tab = Project::getInstance()->getSupplemental();

	for (int i = 0; i < tab->getNumTabs(); i++) {

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

void SynthEditor::openStepSequencer(StepSequencerModule* ssm) {
	SequenceEditor* se = ssm->getEditor();
	Project::getInstance()->getSupplemental()->addTab("Step sequencer", Colours::darkgrey, se, true);
	Project::getInstance()->getSupplemental()->setCurrentTabIndex(Project::getInstance()->getSupplemental()->getNumTabs() - 1);
}

void SynthEditor::openRecorder(AudioRecorderModule* arm) {

	MainTabbedComponent* tab = Project::getInstance()->getSupplemental();

	for (int i = 0; i < tab->getNumTabs(); i++) {

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
	FileChooser chooser("Select file to open", File::getSpecialLocation(File::commonDocumentsDirectory), "*.xml;*.slb", true, true);

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
		std::unique_ptr<XmlElement> xml = XmlDocument(file).getDocumentElement();

		Project::getInstance()->addRecentFile(file.getFullPathName());
#endif
		ValueTree v = ValueTree::fromXml(*xml.get());
		setRunning(false);
		ModuleUtils::loadStructure(root->getModules(), root->getConnections(), &v, this);

		{
			long maxIdx = 0;
			for (auto* m : *root->getModules())
				if (m->getIndex() > maxIdx) maxIdx = m->getIndex();
			PrefabFactory::getInstance()->updateLastModuleIndex(maxIdx);
		}

		for (std::vector<Module*>::iterator it = root->getModules()->begin(); it != root->getModules()->end(); ++it) {
			addAndMakeVisible((*it));
			configureAudioModule((*it), this);
			ImageModule* im;
			if ((im = dynamic_cast<ImageModule*>(*it)) != NULL) {
				im->toBack();
			}
		}

		// Modules and connections are loaded — let the mixer panels mirror any
		// FX PluginModules that sit between a source and an AudioOut into their
		// fxChain[] slot UI.
		if (mixer != nullptr)
			mixer->reconstructAllFXChains();

		Project::getInstance()->setRoot(root.get());

		resized();

		setCurrentLayer(v.getProperty("layer").toString().getIntValue());
		setLocked(v.getProperty("lock").toString().getIntValue() > 0);

		notifyListeners();

		xml = nullptr;

		openTracks(file);

#if JUCE_IOS
		updateProject(url);
#else
		updateProject(file);
#endif

		setRunning(true);
	}
}

void SynthEditor::openTracks(File file)
{

	String path = file.getParentDirectory().getFullPathName();

	String audioDataFileName = file.getFileNameWithoutExtension() + "_audio.xml";
	String trackDataFileName = file.getFileNameWithoutExtension() + "_tracks.xml";

	audioDataFileName = path + "\\" + audioDataFileName;
	trackDataFileName = path + "\\" + trackDataFileName;

	std::unique_ptr<XmlElement> audioXml = XmlDocument(File(audioDataFileName)).getDocumentElement();

	if (audioXml == nullptr)
		return;

	ValueTree audio = ValueTree::fromXml(*audioXml.get());

	// Restore tempo before creating any regions so MidiRegion constructors
	// pick up the correct value via updateTempoFromProject().
	float savedTempo = (float)(double)audio.getProperty("tempo", (double)Project::DEFAULT_TEMPO);
	if (savedTempo > 0.0f) {
		Project::getInstance()->setTempo(savedTempo);
		FastBus::getInstance().publish("tempo", savedTempo);
	}

	audio = audio.getChild(0);

	for (int i = 0; i < audio.getNumChildren(); i++) {
		path = audio.getChild(i).getProperty("path");
		String id = audio.getChild(i).getProperty("refId");

		Project::getInstance()->addAudioFile(id, path);
	}

	std::unique_ptr<XmlElement> xml = XmlDocument(File(trackDataFileName)).getDocumentElement();

	ValueTree v = ValueTree::fromXml(*xml.get());
	setRunning(false);

	vector<TrackIn*> tracks = vector<TrackIn*>();

	for (int i = 0; i < getModule()->getModules()->size(); i++) {

		TrackIn* ti = dynamic_cast<TrackIn*>(getModule()->getModules()->at(i));

		if (ti != nullptr) {
			tracks.push_back(ti);
		}
	}

	if (tracks.size() > 0) {
		for (int i = 0; i < v.getNumChildren(); i++) {

			ValueTree track = v.getChild(i);
			TrackIn* ti = tracks.at(i);

			if (track.getProperty("type") == "audio") {
				Track* t = navigator->addTrack(Track::Type::AUDIO, Project::getInstance()->getSampleRate());
				ti->setTrack(t);
				t->setName(track.getProperty("name"));

				ValueTree regions = track.getChild(0);

				for (int j = 0; j < regions.getNumChildren(); j++) {
					ValueTree region = regions.getChild(j);
					Logger::getCurrentLogger()->writeToLog(region.getProperty("clipRefId"));
					Region* r = t->addRegion(region.getProperty("clipRefId"), File(Project::getInstance()->getAudioPath(region.getProperty("clipRefId"))), Project::getInstance()->getSampleRate(), region.getProperty("sampleOffset").toString().getIntValue());
					Rectangle<int> bounds = r->getBounds();
					bounds.setWidth(region.getProperty("width"));
					r->setBounds(bounds);
				}
			}
			else if (track.getProperty("type") == "midi") {
				juce::Point<int> pos = juce::Point<int>(10, 10);
				Track* t = navigator->addTrack(Track::Type::MIDI, Project::getInstance()->getSampleRate());
				ti->setTrack(t);
				t->setMidiChannel(track.getProperty("midiChannel").toString().getIntValue(), false);
				t->setMidiOutputDevice(track.getProperty("interface").toString());
				loadMidiRegions(track, t);
			}
			else if (track.getProperty("type") == "instrument") {
				juce::Point<int> pos = juce::Point<int>(10, 10);
				Track* t = navigator->addTrack(Track::Type::INSTRUMENT, Project::getInstance()->getSampleRate());
				ti->setTrack(t);
				t->setMidiChannel(track.getProperty("midiChannel").toString().getIntValue(), false);

				// Re-link the PluginModule to the track.
				// The TrackIn's event-out pin (index 5) is connected to the PluginModule's
				// event-in pin. We find the PluginModule by matching against that connected pin.
				auto& tiPins = ti->getPins();
				if (tiPins.size() > 5 && !tiPins.at(5)->getConnections().empty()) {
					Pin* connectedPin = tiPins.at(5)->getConnections().at(0);
					for (auto* mod : *getModule()->getModules()) {
						if (auto* pm = dynamic_cast<PluginModule*>(mod)) {
							for (auto* pin : pm->getPins()) {
								if (pin == connectedPin) {
									t->setPluginModule(pm);
									break;
								}
							}
						}
					}
				}

				loadMidiRegions(track, t);
			}
		}
	}

	setRunning(true);
}

void SynthEditor::loadMidiRegions(juce::ValueTree& track, Track* t) {
	ValueTree regions = track.getChild(0);

	for (int j = 0; j < regions.getNumChildren(); j++) {
		ValueTree region = regions.getChild(j);
		MidiRegion* mr = t->addMidiRegion(
			Project::getInstance()->getSampleRate(),
			int(region.getProperty("sampleOffset")),
			int(region.getProperty("numSamples"))
		);

		Rectangle<int> bounds = mr->getBounds();
		bounds.setWidth(int(region.getProperty("width")));
		mr->setBounds(bounds);

		ValueTree events = region.getChild(0);
		int numEvents = events.getNumChildren();
		DBG("Loading " + String(numEvents) + " events.");

		int loadedCount = 0;
		for (int k = 0; k < numEvents; k++) {
			ValueTree event = events.getChild(k);
			String eventType = event.getProperty("type").toString().toLowerCase().trim();

			DBG("Processing event " + String(k) + ": " + eventType);

			MidiMessage msg;
			bool validMessage = false;

			if (eventType == "noteon") {
				msg = MidiMessage::noteOn(
					int(event.getProperty("channel")),
					int(event.getProperty("note")),
					float(event.getProperty("velocity")) / 127.0f
				);
				validMessage = true;
			}
			else if (eventType == "noteoff") {
				msg = MidiMessage::noteOff(
					int(event.getProperty("channel")),
					int(event.getProperty("note")),
					0.0f
				);
				validMessage = true;
			}
			else if (eventType == "control") {
				msg = MidiMessage::controllerEvent(
					int(event.getProperty("channel")),
					int(event.getProperty("controller")),
					int(event.getProperty("value"))
				);
				validMessage = true;
			}
			else if (eventType == "pitch") {
				msg = MidiMessage::pitchWheel(
					int(event.getProperty("channel")),
					int(event.getProperty("pitch"))
				);
				validMessage = true;
			}
			else {
				DBG("WARNING: Unknown event type: " + eventType);
			}

			if (validMessage) {
				int timestamp = int(event.getProperty("timestamp"));
				mr->addMessage(new MidiMessage(msg), timestamp, timestamp);
				loadedCount++;
			}
		}

		DBG("Successfully loaded " + String(loadedCount) + " of " + String(numEvents) + " events.");
	}
}

void SynthEditor::configureAudioModule(Module* m, ChangeBroadcaster* broadcaster) {
	if (m == nullptr || mixer == nullptr) return;

	AudioManager* am = AudioManager::getInstance();
	if (am == nullptr) {
		broadcaster->addChangeListener(m);
		return;
	}

	auto* device = am->getDeviceManager()->getCurrentAudioDevice();

	AudioOut* out;

	if ((out = dynamic_cast<AudioOut*>(m)) != NULL) {
		mixer->getOutputChannels().push_back(out);
		int idx = static_cast<int>(mixer->getOutputChannels().size()) - 1;
		String channelName;
		if (device != nullptr) {
			auto names = device->getOutputChannelNames();
			if (idx >= 0 && idx < names.size())
				channelName = names[idx];
			else
				channelName = "Out " + String(idx + 1);
		} else {
			channelName = "Out " + String(idx + 1);
		}
		int channelIndex = addChannel(channelName, Mixer::Channel::Type::OUT);
		out->setChannelIndex(channelIndex);
		mixer->setVolume(channelIndex, out->getVolume());
		mixer->setModule(channelIndex, out);
	}

	AudioIn* in;

	if ((in = dynamic_cast<AudioIn*>(m)) != NULL) {
		if (device != nullptr && device->getInputChannelNames().size() > 0) {
			mixer->getInputChannels().push_back(in);
			int idx = static_cast<int>(mixer->getInputChannels().size()) - 1;
			auto names = device->getInputChannelNames();
			String channelName = (idx >= 0 && idx < names.size()) ? names[idx] : ("In " + String(idx + 1));
			int channelIndex = addChannel(channelName, Mixer::Channel::Type::IN);
			in->setChannelIndex(channelIndex);
			mixer->setVolume(channelIndex, in->getVolume());
			mixer->setModule(channelIndex, in);
		}
	}

	AuxOut* aux;

	if ((aux = dynamic_cast<AuxOut*>(m)) != NULL) {
		mixer->getAuxChannels().push_back(aux);
		String channelName = "Aux " + String(mixer->getAuxChannels().size());
		int channelIndex = addChannel(channelName, Mixer::Channel::Type::AUX);
		aux->setChannelIndex(channelIndex);
		mixer->setVolume(channelIndex, aux->getVolume());
		mixer->setModule(channelIndex, aux);
	}

	broadcaster->addChangeListener(m);
}

juce::Point<int> SynthEditor::findFreePosition(juce::Point<int> desired, int w, int h) const
{
    const int margin = 10;
    const int step   = 20;

    auto isFree = [&](juce::Point<int> p) -> bool {
        juce::Rectangle<int> candidate(p.x, p.y, w, h);
        for (auto* m : *root->getModules()) {
            if (m->getBounds().expanded(margin).intersects(candidate))
                return false;
        }
        return true;
    };

    if (isFree(desired))
        return desired;

    // Scan right, then below, then diagonal — preference: stay near the original Y
    for (int dist = step; dist < 3000; dist += step) {
        juce::Point<int> candidates[] = {
            { desired.x + dist, desired.y },
            { desired.x,        desired.y + dist },
            { desired.x + dist, desired.y + dist },
        };
        for (auto& p : candidates)
            if (isFree(p)) return p;
    }
    return desired;
}

juce::Point<int> SynthEditor::getNextTrackStartPosition() const
{
    if (root->getModules()->empty())
        return { 20, 40 };

    // Find the bottom edge of all current modules, then add padding
    int maxBottom = 0;
    for (auto* m : *root->getModules())
        maxBottom = juce::jmax(maxBottom, m->getBottom());

    return { 20, maxBottom + 30 };
}

void SynthEditor::addVirtualInstrumentTrack()
{
	PluginBrowserComponent::show([this](juce::String selectedPlugin)
	{
		Point<int> pos = getNextTrackStartPosition();
		TrackUtil::createVirtualInstrumentTrack(selectedPlugin, pos, navigator, this);
		repaint();
	});
}

void SynthEditor::setTab(juce::TabbedComponent* t) {
	this->tab = t;
}

void SynthEditor::addConnection(const MouseEvent& e, Module* source) {
	AddConnectionAction* ac = new AddConnectionAction(this, source);
	Project::getInstance()->getUndoManager()->beginNewTransaction();
	Project::getInstance()->getUndoManager()->perform(ac);
	resized();
	repaint();
}

void SynthEditor::setModule(std::unique_ptr<Module> m, bool deleteWhenRemoved) {

	if (root != nullptr) {
		root = nullptr;
	}
	selectionModel.setRoot(m.get());
	this->root = std::move(m);
	isRoot = false;
	this->deleteModuleWhenRemoved = deleteWhenRemoved;
	for (std::vector<Module*>::iterator it = root->getModules()->begin(); it != root->getModules()->end(); ++it) {
		addAndMakeVisible((*it));
	}

	repaint();
}

void SynthEditor::setModuleRef(Module* m) {
    if (root != nullptr)
        root = nullptr;
    selectionModel.setRoot(m);
    // Non-owning shared_ptr: no-op deleter ensures we never delete the module
    this->root = std::shared_ptr<Module>(m, [](Module*){});
    isRoot = false;
    deleteModuleWhenRemoved = false;
    for (auto* mod : *root->getModules())
        addAndMakeVisible(mod);
    repaint();
}

Module* SynthEditor::getModule() {
	return root.get();
}

void SynthEditor::prepareToPlay(int samplesPerBlockExpected, double sampleRate) {
	this->_sampleRate = sampleRate;
	this->bufferSize = samplesPerBlockExpected;
	Project::getInstance()->setSampleRate(sampleRate);
	Project::getInstance()->setBufferSize(samplesPerBlockExpected);

	Logger::writeToLog("SynthEditor using sample rate " + String(sampleRate) + " kHz and buffer size of " + String(samplesPerBlockExpected) + " bytes.");

	sendChangeMessage();

}

bool SynthEditor::isInterestedInDragSource(const SourceDetails& dragSourceDetails) {
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
		DuplicateModuleAction* am = new DuplicateModuleAction(this, pos, m);
		Project::getInstance()->getUndoManager()->beginNewTransaction();
		Project::getInstance()->getUndoManager()->perform(am);
	}
}

void SynthEditor::addEditorListener(EditorListener* listener) {
	this->listener.push_back(listener);
}

void SynthEditor::notifyListeners() {

	if (this->selectionModel.getSelectedModule() != nullptr) {

		for (int i = 0; i < listener.size(); i++) {
			listener.at(i)->selectionChanged(this->selectionModel.getSelectedModule());
		}
	}
}

void SynthEditor::notifyDirtyListeners() {
	for (int i = 0; i < listener.size(); i++) {
		listener.at(i)->dirtyChanged(isDirty());
	}
}

bool SynthEditor::isDirty() {
	return dirty;
}

void SynthEditor::setDirty(bool dirty) {
	this->dirty = dirty;
	for (int i = 0; i < tab->getNumTabs(); i++) {

		Viewport* view = dynamic_cast<Viewport*>(tab->getTabContentComponent(i));

		if (view != nullptr) {

			SynthEditor* editor = dynamic_cast<SynthEditor*>(view->getViewedComponent());

			if (editor != nullptr && editor == this) {

				if (dirty) {
					if (Project::getInstance()->isNewFile()) {
						tab->setTabName(i, "untitled*");
					}
					else {
						tab->setTabName(i, Project::getInstance()->getCurrentFileName() + "*");
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
		format.writeImageToStream(image, *fos);
		delete fos;
	}
}

void SynthEditor::changeListenerCallback(juce::ChangeBroadcaster* source) {

	ExtendedFileBrowser* efb = dynamic_cast<ExtendedFileBrowser*>(source);

	if (efb != nullptr) {

		File* f = efb->getSelectedFile();

		if (f->getFullPathName().endsWith("slb") || f->getFullPathName().endsWith("xml")) {
			FileInputStream* fis = new FileInputStream(*f);
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
void SynthEditor::getAllCommands(Array<CommandID>& commands) {
	commands.add({ SynthEditor::CommandIds::NEW });
	// commands.add({ SynthEditor::CommandIds::ADD_MODULE });
	// commands.add({ SynthEditor::CommandIds::DUPLICATE });
	commands.add({ SynthEditor::CommandIds::DELETE_SELECTED });
	commands.add({ SynthEditor::CommandIds::SAVE });
	commands.add({ SynthEditor::CommandIds::LOAD });
	commands.add({ SynthEditor::CommandIds::EXPORT_AUDIO });
	commands.add({ SynthEditor::CommandIds::LOAD_MODULE });
	commands.add({ SynthEditor::CommandIds::SAVE_MODULE });
	commands.add({ SynthEditor::CommandIds::SAVE_SCREENSHOT });
	commands.add({ SynthEditor::CommandIds::ALIGN_X });
	commands.add({ SynthEditor::CommandIds::ALIGN_Y });
	commands.add({ SynthEditor::CommandIds::ALIGN_TO_GRID });
	commands.add({ SynthEditor::CommandIds::RESET_GUI_POS });
}

void SynthEditor::getCommandInfo(CommandID commandID, ApplicationCommandInfo& result) {
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
	case SynthEditor::CommandIds::ALIGN_TO_GRID:
		result.setInfo("Align to grid", String(), String(), 0);
		break;
	case SynthEditor::CommandIds::RESET_GUI_POS:
		result.setInfo("Reset UI position", String(), String(), 0);
		break;
	case SynthEditor::CommandIds::EXPORT_AUDIO:
		result.setInfo("Export audio", String(), String(), 0);
		break;
	}
};

bool SynthEditor::perform(const InvocationInfo& info) {

	if (info.commandID == SynthEditor::CommandIds::NEW) {
		setRunning(false);
		cleanUp();
		newFile();
		updateProject(File());
		Project::getInstance()->setNewFile(true);
		setRunning(true);
		return true;
	}
	else if (info.commandID == SynthEditor::CommandIds::ADD_MODULE) {
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
	else if (info.commandID == SynthEditor::CommandIds::LOAD) {
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
	else if (info.commandID == SynthEditor::CommandIds::SAVE_MODULE) {
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
	else if (info.commandID == SynthEditor::CommandIds::DELETE_SELECTED) {
		RemoveSelectedAction* rsa = new RemoveSelectedAction(this);
		Project::getInstance()->getUndoManager()->beginNewTransaction();
		Project::getInstance()->getUndoManager()->perform(rsa);
		return true;
	}
	else if (info.commandID == SynthEditor::CommandIds::ALIGN_Y) {
		alignSelectedY();
		return true;
	}
	else if (info.commandID == SynthEditor::CommandIds::ALIGN_X) {
		alignSelectedX();
		return true;
	}
	else if (info.commandID == SynthEditor::CommandIds::ALIGN_Y) {
		alignToGrid();
		return true;
	}
	else if (info.commandID == SynthEditor::CommandIds::RESET_GUI_POS) {
		resetGUIPosition();
		return true;
	}
	else if (info.commandID == SynthEditor::CommandIds::EXPORT_AUDIO) {
		exportAudio();
		return true;
	}

	return true;
}

void SynthEditor::alignSelectedY() {
	AlignVerticalAction* ava = new AlignVerticalAction(this);	
	Project::getInstance()->getUndoManager()->beginNewTransaction();
	Project::getInstance()->getUndoManager()->perform(ava);
}

void SynthEditor::alignSelectedX() {
	AlignHorizontalAction* aha = new AlignHorizontalAction(this);
	Project::getInstance()->getUndoManager()->beginNewTransaction();
	Project::getInstance()->getUndoManager()->perform(aha);
}

void SynthEditor::alignToGrid()
{
	// align all selected modules to the grid

	for (int i = 0; i < selectionModel.getSelectedModules().size(); i++) {
		int x = selectionModel.getSelectedModules().at(i)->getBounds().getX();
		int y = selectionModel.getSelectedModules().at(i)->getBounds().getY();

		x = UITools::snap(x, 25, 5);
		y = UITools::snap(y, 25, 5);

		selectionModel.getSelectedModules().at(i)->setTopLeftPosition(x,y);

	}
	repaint();
}

void SynthEditor::resetGUIPosition() {
	for (int i = 0; i < selectionModel.getSelectedModules().size(); i++) {
		selectionModel.getSelectedModules().at(i)->resetUIPosition();
	}
}

void SynthEditor::updateProject(URL url) {

	Project::getInstance()->setCurrentFilePath(url.toString(false));
	Project::getInstance()->setCurrentFileName(url.getFileName());

	for (int i = 0; i < tab->getNumTabs(); i++) {

		Viewport* view = dynamic_cast<Viewport*>(tab->getTabContentComponent(i));

		if (view != nullptr) {

			SynthEditor* editor = dynamic_cast<SynthEditor*>(view->getViewedComponent());

			if (editor != nullptr && editor == this) {
				if (url.isEmpty()) {
					tab->setTabName(i, "untitled");
				}
				else {
					tab->setTabName(i, Project::getInstance()->getCurrentFileName());
				}

				break;
			}
		}
	}

	for (int i = 0; i < listener.size(); i++) {
		listener.at(i)->fileChanged(url.toString(false));
	}
}

void SynthEditor::setNavigator(TrackNavigator* navigator)
{
	this->navigator = navigator;
}

void SynthEditor::exportAudio()
{
	// TODO: implement export audio dialog
}

void SynthEditor::updateProject(File file) {

	Project::getInstance()->setCurrentFilePath(file.getFullPathName());
	Project::getInstance()->setCurrentFileName(file.getFileName());

	for (int i = 0; i < tab->getNumTabs(); i++) {

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

	for (int i = 0; i < listener.size(); i++) {
		listener.at(i)->fileChanged(file.getFullPathName());
	}
}

void SynthEditor::cutConnectionAtPosition(Point<int> position)
{
	for (int i = root->getConnections()->size() - 1; i >= 0; i--) {
		Connection* c = root->getConnections()->at(i);

		if (c->source != nullptr && c->target != nullptr) {
			int x1 = c->source->getX() + c->a->x;
			int y1 = c->source->getY() + c->a->y + 5;
			int x2 = c->target->getX() + c->b->x;
			int y2 = c->target->getY() + c->b->y + 5;

			// calculate distance from point to line
			Line<float> line(x1, y1, x2, y2);
			Point<float> nearestPoint;
			float distance = line.getDistanceFromPoint(position.toFloat(), nearestPoint);

			if (distance <= 15.0f) { // 15 Pixel tolerance
				removeConnection(c);
				break; // just one connection at a time
			}
		}
	}
	repaint();
}

void SynthEditor::cutConnectionsAlongLine(Point<int> start, Point<int> end)
{
	Line<float> cutLine(start.toFloat(), end.toFloat());

	for (int i = root->getConnections()->size() - 1; i >= 0; i--) {
		Connection* c = root->getConnections()->at(i);

		if (c->source != nullptr && c->target != nullptr) {
			int x1 = c->source->getX() + c->a->x;
			int y1 = c->source->getY() + c->a->y + 5;
			int x2 = c->target->getX() + c->b->x;
			int y2 = c->target->getY() + c->b->y + 5;

			Line<float> connectionLine(x1, y1, x2, y2);
			Point<float> intersection;

			if (connectionLine.intersects(cutLine, intersection)) {
				removeConnection(c);
			}
		}
	}
}

void SynthEditor::removeConnection(Connection* c)
{
	// remove from modules
	if (c->a != nullptr) {
		auto& connections = c->a->getConnections();
		auto it = std::find(connections.begin(), connections.end(), c->b);
		if (it != connections.end()) {
			connections.erase(it);
		}
	}

	if (c->b != nullptr) {
		auto& connections = c->b->getConnections();
		auto it = std::find(connections.begin(), connections.end(), c->a);
		if (it != connections.end()) {
			connections.erase(it);
		}
	}

	// remove from root
	auto& rootConnections = *root->getConnections();
	auto it = std::find(rootConnections.begin(), rootConnections.end(), c);
	if (it != rootConnections.end()) {
		rootConnections.erase(it);
	}

	delete c;
}