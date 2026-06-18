
#include "SynthEditor.h"
#include "MidiGate.h"
#include "MidiNote.h"
#include "MidiOut.h"
#include "AudioOut.h"
#include "AudioIn.h"
#include "TrackIn.h"
#include "Tracks/AudioRegion.h"
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
#include "ProjectLoadProgressDialog.h"
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
#include "Actions/AddTrackAction.h"
#include "Actions/TrackDeleteAction.h"

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

	// ~30 Hz: drives the pin-activity glow (event fade + audio peak decay).
	startTimer(33);
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
	// Focus-Setup parallel zum Default-Ctor — sonst hat ein per-openSubModule
	// erzeugter Sub-Editor nie Keyboard-Focus, modifierKeysChanged feuert
	// nicht (isLeftShiftDown bleibt false), und Shift-A im Sub-Modul oeffnet
	// nie das Add-Menue.
	setMouseClickGrabsKeyboardFocus(true);
	setWantsKeyboardFocus(true);
	setInterceptsMouseClicks(true, true);
	addChildComponent(root.get());

	// ~30 Hz: drives the pin-activity glow (event fade + audio peak decay).
	startTimer(33);

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
	updateRootLevelGroupEditMode();
	repaint();
}

bool SynthEditor::isLocked() {
	return this->locked;
}

void SynthEditor::setModuleLocked(Module* m, bool locked) {

	if (m == nullptr) return;

	m->setLocked(locked);

	// Recurse so surfaced controls (which sit several levels below the editor
	// root) actually receive the lock signal — without this, sliders inside
	// nested modules stay frozen when the user toggles play mode.
	for (auto* child : *m->getModules()) {
		if (child != nullptr)
			setModuleLocked(child, locked);
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

	// Logical canvas size: 1.5× Breite, 3× Hoehe.
	// Mehr vertikales Headroom, damit beim Bouncen/Debuggen mehr Module
	// gleichzeitig sichtbar sind. Hoehe darf groesser sein als die Breite —
	// der Viewport scrollt vertikal, kein Layout-Code haengt am Seitenverhaeltnis.
	//
	// NICHT zusaetzlich mit editorZoom multiplizieren — setTransform unten
	// skaliert das Rendering bereits, und getBounds() bleibt laut JUCE-Doc
	// untransformiert. Frueher stand hier `* editorZoom`, das hat die Editor-
	// Flaeche quadratisch mit dem Zoom skaliert (zoom² statt zoom) und die
	// Module relativ zur Hintergrund-Flaeche zu gross/klein wirken lassen.
	const int logicalW = (int)(getParentWidth() * 1.5);
	const int logicalH = (int)(getParentHeight() * 3.0);
	setSize(logicalW, logicalH);

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
		// Fire addConnection exactly once, with the module that actually owns
		// the drag-start pin. The old code iterated `if (m->isSelected())` over
		// every module and called addConnection per match — if the user had a
		// multi-module selection before grabbing a pin, every selected module
		// would trigger a separate AddConnectionAction. Each action then scans
		// the editor for "any selected pin", and stale pin-selection state
		// could wire the same input twice or produce ghost connections.
		Module* sourceModule = nullptr;
		if (startPin != nullptr) {
			for (auto* m : *root->getModules()) {
				if (m == nullptr) continue;
				for (auto* p : m->pins) {
					if (p == startPin) { sourceModule = m; break; }
				}
				if (sourceModule != nullptr) break;
			}
		}

		if (sourceModule != nullptr) {
			addConnection(e, sourceModule);
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

void SynthEditor::mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& wheel)
{
	// Cmd/Ctrl + wheel zooms the editor canvas. Plain wheel falls through to
	// the parent Viewport so vertical scrolling keeps working as before.
	if (!(e.mods.isCommandDown() || e.mods.isCtrlDown())) {
		Component::mouseWheelMove(e, wheel);
		return;
	}

	const float factor = 1.0f + wheel.deltaY * 0.25f;
	const float newZoom = juce::jlimit(kMinZoom, kMaxZoom, editorZoom * factor);
	if (std::abs(newZoom - editorZoom) < 1.0e-4f)
		return;

	setEditorZoom(newZoom, e.getPosition());
}

void SynthEditor::setEditorZoom(float newZoom, juce::Point<int> anchorInLocalCoords)
{
	newZoom = juce::jlimit(kMinZoom, kMaxZoom, newZoom);
	if (std::abs(newZoom - editorZoom) < 1.0e-4f)
		return;

	// Anchor the zoom around `anchorInLocalCoords` (local untransformed coords).
	// The visual position of that point is `anchor * editorZoom`. After
	// changing zoom it becomes `anchor * newZoom`. Compensate by shifting
	// the parent Viewport's view position by the visual delta.
	juce::Viewport* vp = findParentComponentOfClass<juce::Viewport>();
	const juce::Point<float> anchorF = anchorInLocalCoords.toFloat();
	const juce::Point<float> visualBefore = anchorF * editorZoom;
	const juce::Point<float> visualAfter  = anchorF * newZoom;
	const juce::Point<int> visualDelta = (visualAfter - visualBefore).toInt();

	editorZoom = newZoom;
	setTransform(juce::AffineTransform::scale(editorZoom));

	// Recompute bounds so the Viewport sees the scaled canvas size and the
	// scrollbars cover the full zoomed-in content.
	resized();

	if (vp != nullptr) {
		const auto pos = vp->getViewPosition();
		vp->setViewPosition(pos + visualDelta);
	}

	repaint();
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

namespace {
	// ID offset for user-module menu items — kept well above the highest
	// PrefabFactory ID so the two ranges can't collide.
	constexpr int kUserModuleMenuIdBase = 100000;

	juce::File getUserModulesDir()
	{
		auto dir = juce::File::getSpecialLocation(juce::File::userHomeDirectory)
					   .getChildFile(".Synthlab/modules");
		if (!dir.exists())
			dir.createDirectory();
		return dir;
	}

	struct UserModuleFileCmp
	{
		int compareElements(const juce::File& a, const juce::File& b) const
		{
			return a.getFileName().compareNatural(b.getFileName());
		}
	};

	// Underscores in filenames are treated as word separators: each segment
	// becomes its own word, joined by spaces, with the first character of
	// each segment upper-cased. The remaining characters keep their original
	// case (so "LFO_module" reads as "LFO Module", not "Lfo Module").
	juce::String prettifyUserModuleName(const juce::String& raw)
	{
		juce::StringArray parts;
		parts.addTokens(raw, "_", {});
		parts.removeEmptyStrings();

		for (auto& p : parts)
			if (p.isNotEmpty())
				p = p.substring(0, 1).toUpperCase() + p.substring(1);

		return parts.joinIntoString(" ");
	}

	// Recursively populate `menu` from `dir`: subfolders become submenus
	// (skipped if they contain no .slb file anywhere underneath, so the user
	// does not see empty branches), .slb files become leaf items. `files` is
	// the flat lookup table — its index plus kUserModuleMenuIdBase is the
	// assigned menu ID.
	void buildUserModulesMenu(const juce::File& dir,
							  juce::PopupMenu& menu,
							  std::vector<juce::File>& files)
	{
		juce::Array<juce::File> subdirs;
		dir.findChildFiles(subdirs, juce::File::findDirectories, false);
		UserModuleFileCmp cmp;
		subdirs.sort(cmp);

		for (const auto& sub : subdirs)
		{
			juce::PopupMenu sm;
			const size_t beforeCount = files.size();
			buildUserModulesMenu(sub, sm, files);
			if (files.size() != beforeCount)
				menu.addSubMenu(prettifyUserModuleName(sub.getFileName()), sm);
		}

		juce::Array<juce::File> slbFiles;
		dir.findChildFiles(slbFiles, juce::File::findFiles, false, "*.slb");
		slbFiles.sort(cmp);

		for (const auto& f : slbFiles)
		{
			const int id = kUserModuleMenuIdBase + (int) files.size();
			files.push_back(f);
			menu.addItem(id, prettifyUserModuleName(f.getFileNameWithoutExtension()));
		}
	}
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

	// Scan ~/.Synthlab/modules/ — user-saved .slb modules become a "User
	// Modules" submenu at the bottom of the Add menu. Subfolders nest as
	// submenus, mirroring the on-disk layout.
	std::vector<juce::File> userModuleFiles;
	PopupMenu userModulesMenu;
	buildUserModulesMenu(getUserModulesDir(), userModulesMenu, userModuleFiles);
	if (!userModuleFiles.empty()) {
		prefabMenu.addSeparator();
		prefabMenu.addSubMenu("User Modules", userModulesMenu, true);
	}

	const int result = prefabMenu.show();
	prefabMenu.setLookAndFeel(nullptr);


	if (result <= 0)
		return;

	if (result >= kUserModuleMenuIdBase) {
		const int idx = result - kUserModuleMenuIdBase;
		if (idx < 0 || idx >= (int) userModuleFiles.size())
			return;

		const File& file = userModuleFiles[(size_t) idx];

		std::unique_ptr<XmlElement> xml = XmlDocument(file).getDocumentElement();
		if (xml == nullptr)
			return;

		ValueTree v = ValueTree::fromXml(*xml);

		setRunning(false);
		// Block the audio thread — see openFile(File) for the full rationale.
		// loadFromXml itself runs push_backs into a brand-new Module's vector
		// (no race), but the subsequent push_back into root->getModules()
		// inside AddModuleAction reallocates the project's tree while the
		// audio thread iterates it.
		const juce::ScopedLock graphBarrier(
			AudioManager::getInstance()->getProcessGraphLock());

		Module* loaded = ModuleUtils::loadFromXml(v, this);
		if (loaded == nullptr) {
			setRunning(true);
			return;
		}
		loaded->setName(v.getProperty("name"));

		// Bump PrefabFactory past every index inside the just-loaded module so
		// the next prefab created doesn't collide with a nested submodule's
		// index (see updateLastModuleIndex call in openFile for the rationale).
		{
			long localMax = loaded->getIndex();
			const long childMax = ModuleUtils::maxIndexInTree(loaded->getModules());
			if (childMax > localMax) localMax = childMax;
			PrefabFactory::getInstance()->updateLastModuleIndex(localMax);
		}

		// Heal index collisions inside the loaded .slb (e.g. two non-prefab
		// groups both saved at index=0 because Module::index was uninitialized
		// in older builds). Without this the loaded file keeps misbehaving:
		// the second group can't be picked as a drag target by
		// AddConnectionAction, and loadConnections cross-links connections
		// that mention the shared index.
		{
			auto collisions = ModuleUtils::findIndexCollisions(loaded->getModules());
			for (auto& kv : collisions) {
				juce::String names;
				for (auto& n : kv.second) names << n << " ";
				Logger::writeToLog("WARN: .slb user-module collision idx="
					+ juce::String(kv.first) + " shared by: " + names
					+ "— auto-renumbering duplicate.");
			}
			if (ModuleUtils::autoFixIndexCollisions(loaded->getModules()) > 0)
				setDirty(true);
		}

		// Reuse the existing prefab path so this is a single undoable step.
		// Pre-setting the module short-circuits the PrefabFactory lookup
		// inside perform(); the moduleId only needs to clear the `< 1` guard.
		AddModuleAction* am = new AddModuleAction(this, position, 1);
		am->setModule(loaded);
		Project::getInstance()->getUndoManager()->beginNewTransaction();
		Project::getInstance()->getUndoManager()->perform(am);
		setRunning(true);
		return;
	}

	AddModuleAction* am = new AddModuleAction(this, position, result);
	Project::getInstance()->getUndoManager()->beginNewTransaction();
	Project::getInstance()->getUndoManager()->perform(am);
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
			if (!confirmDiscardIfDirty()) {
				menu.setLookAndFeel(nullptr);
				return;
			}
			FileInputStream* fis = new FileInputStream(file);
			String data = fis->readEntireStreamAsString();
			delete fis;
			setRunning(false);
			cleanUp();
			newFile(/*askIfDirty=*/ false);
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
    // The base Module(name) ctor doesn't take audio settings; without this
    // the group's own per-block loops (Module::process terminal forwarding)
    // could fall back to default 0 if the header default ever drifts.
    group->setSampleRate(getSampleRate());
    group->setBuffersize(getBufferSize());

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
        if (conn->a == nullptr || conn->b == nullptr) continue;

        // Connection::source/target reflect the order in which the user clicked
        // the pins when drawing the wire, not the data-flow direction. Classify
        // by pin direction so that OUT is always the producer and IN the consumer.
        Module* prodMod; Pin* prodPin;
        Module* consMod; Pin* consPin;
        if (conn->a->direction == Pin::Direction::OUT && conn->b->direction == Pin::Direction::IN) {
            prodMod = conn->source; prodPin = conn->a;
            consMod = conn->target; consPin = conn->b;
        } else if (conn->a->direction == Pin::Direction::IN && conn->b->direction == Pin::Direction::OUT) {
            prodMod = conn->target; prodPin = conn->b;
            consMod = conn->source; consPin = conn->a;
        } else {
            continue;
        }

        bool prodIn = isInSel(prodMod);
        bool consIn = isInSel(consMod);

        if (prodIn && consIn) {
            // Pure internal: move connection to the group
            auto& rc = *root->getConnections();
            rc.erase(std::remove(rc.begin(), rc.end(), conn), rc.end());
            group->addConnection(conn);

        } else if (!prodIn && consIn) {
            // Crossing IN: external producer feeds internal consumer
            auto* term = dynamic_cast<TerminalModule*>(
                PrefabFactory::getInstance()->getPrefab(83, getSampleRate(), getBufferSize()));
            if (!term) continue;

            term->setType(consPin->getType());

            Pin* extPin = new Pin(consPin->getType());
            term->getPins().at(0)->setTerminal(extPin);
            term->getPins().at(0)->setType(consPin->getType());
            extPin->listeners.push_back(term);
            extPin->setName(term->getName());
            term->addChangeListener(extPin);

            group->addPin(Pin::Direction::IN, extPin);
            term->setIndex(extPin->index);

            // Position terminal on the left edge of the group interior
            term->setSize(80, 40);
            term->setTopLeftPosition(4, inTermY);
            inTermY += 44;

            // Internal connection: term.out → consumer.consPin
            auto* internalConn = new Connection(term, term->getPins().at(0), consMod, consPin);
            consPin->getConnections().push_back(term->getPins().at(0));
            // Remove old external producer pin from consPin's connections list
            auto& cc = consPin->getConnections();
            cc.erase(std::remove(cc.begin(), cc.end(), prodPin), cc.end());

            group->getModules()->push_back(term);
            group->addConnection(internalConn);

            // Redirect external connection: external producer → group's IN pin
            extPin->getConnections().push_back(prodPin);
            conn->source = prodMod; conn->a = prodPin;
            conn->target = group;   conn->b = extPin;

        } else if (prodIn && !consIn) {
            // Crossing OUT: internal producer feeds external consumer
            Pin* extPin = nullptr;

            if (outTerminalForPin.count(prodPin) == 0) {
                // First time we see this producer pin crossing out: create an OUT terminal
                auto* term = dynamic_cast<TerminalModule*>(
                    PrefabFactory::getInstance()->getPrefab(84, getSampleRate(), getBufferSize()));
                if (!term) continue;

                term->setType(prodPin->getType());

                extPin = new Pin(prodPin->getType());
                term->getPins().at(0)->setTerminal(extPin);
                term->getPins().at(0)->setType(prodPin->getType());
                extPin->listeners.push_back(term);
                extPin->setName(term->getName());
                term->addChangeListener(extPin);

                group->addPin(Pin::Direction::OUT, extPin);
                term->setIndex(extPin->index);

                // Position terminal on the right edge of the group interior
                term->setSize(80, 40);
                term->setTopLeftPosition(groupW - 84, outTermY);
                outTermY += 44;

                // Internal connection: producer.prodPin → term.in
                auto* internalConn = new Connection(prodMod, prodPin, term, term->getPins().at(0));
                term->getPins().at(0)->getConnections().push_back(prodPin);

                group->getModules()->push_back(term);
                group->addConnection(internalConn);

                outTerminalForPin[prodPin] = extPin;
            } else {
                extPin = outTerminalForPin[prodPin];
            }

            // Redirect external connection: external consumer's IN pin now reads from group's OUT pin
            auto& cc = consPin->getConnections();
            cc.erase(std::remove(cc.begin(), cc.end(), prodPin), cc.end());
            cc.push_back(extPin);

            conn->source = group;   conn->a = extPin;
            conn->target = consMod; conn->b = consPin;
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

	// Idempotent: enter/exit nur bei tatsaechlichem Zustandswechsel, sonst
	// koennen aufeinanderfolgende setRunning(false)-Calls den AudioCallback-
	// Lock rekursiv mehrfach betreten und ein spaeteres setRunning(true)
	// haelt den Lock weiter — Audio bleibt dann stumm bzw. spaetere
	// File-Open-Sequenzen verbleiben in einem inkonsistenten Lock-Zaehlerstand.
	if (this->running == running)
		return;

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
	if (root != nullptr) {
		std::function<void(Module*)> walk = [&](Module* m) {
			for (auto* p : m->getPins()) {
				if (p == nullptr) continue;
				// Audio activity is a sampled-peak with per-tick decay; events
				// already write their timestamp, no per-tick work needed.
				if (p->getType() == Pin::Type::AUDIO) {
					p->updateAudioPeak();
				}
			}
			if (m->getModules() != nullptr) {
				for (auto* child : *m->getModules()) {
					if (child != nullptr) walk(child);
				}
			}
		};
		walk(root.get());
	}
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

namespace {
	// Wrapper for the controls of one source module surfaced at the editor root.
	// In play mode (editor locked) we don't catch the click — slider clicks go
	// straight to the child controls. In edit mode (unlocked) the click is used
	// for layout: clicking on a child drags that child within the group, clicking
	// on the title or empty border drags the whole group across the editor.
	class RootLevelGroup : public juce::GroupComponent,
	                       public juce::ChangeListener {
	public:
		RootLevelGroup(const juce::String& title, Module* sourceModule, SynthEditor* editor)
			: juce::GroupComponent(title, title), sourceModule(sourceModule), editor(editor) {
			setColour(juce::GroupComponent::outlineColourId, juce::Colours::grey);
			setColour(juce::GroupComponent::textColourId, juce::Colours::white);

			constrainer.setMinimumSize(80, 60);
			resizer.reset(new juce::ResizableCornerComponent(this, &constrainer));
			addAndMakeVisible(*resizer);
			applyEditMode();
		}

		~RootLevelGroup() override {
			if (activeSelector != nullptr)
				activeSelector->removeChangeListener(this);
		}

		Module* getSourceModule() const { return sourceModule; }
		Module* getInstrumentAncestor() const { return instrumentAncestor; }
		void setInstrumentAncestor(Module* instr) { instrumentAncestor = instr; }

		// juce::GroupComponent's default hitTest excludes the body so clicks fall
		// through to children. We need the body to receive clicks for layout drag,
		// otherwise unlocked controls (which don't intercept) leak through to the
		// editor and we lose the drag entirely.
		bool hitTest(int, int) override { return true; }

		// Fill the body with the user-chosen background colour (if any) before
		// GroupComponent draws its outline + title on top. Without a colour set
		// the body stays transparent and the editor background shows through.
		void paint(juce::Graphics& g) override {
			if (sourceModule != nullptr && sourceModule->hasSurfaceBackgroundColour())
				g.fillAll(sourceModule->getSurfaceBackgroundColour());
			juce::GroupComponent::paint(g);
		}

		void resized() override {
			juce::GroupComponent::resized();
			if (resizer != nullptr) {
				resizer->setBounds(getWidth() - 12, getHeight() - 12, 12, 12);
				resizer->toFront(false);
			}
			// User dragging the corner triggers JUCE setBounds → resized; persist
			// the new dimensions onto the source module so the next layer toggle
			// (or save/load) restores the same group size.
			if (sourceModule != nullptr) {
				sourceModule->setSurfaceSize({ getWidth(), getHeight() });
			}
		}

		// Re-evaluate whether the resizer should be active, based on the editor's
		// current lock state. Called on construction and from SynthEditor::setLocked.
		// Fuer Instrument-Panels gilt eine invertierte Regel: deren Layout darf
		// auch im Lock-Modus bewegt werden, weil das Panel die "Spielflaeche" des
		// Instruments ist und der User es als Performance-Layout positionieren
		// will, waehrend der Rest des Projekts gesperrt bleibt.
		void applyEditMode() {
			if (resizer == nullptr || editor == nullptr) return;
			const bool layoutEnabled = isLayoutEditable();
			resizer->setVisible(layoutEnabled);
			resizer->setInterceptsMouseClicks(layoutEnabled, layoutEnabled);

			// Frueher haben wir hier child->setLocked(true) +
			// setInterceptsMouseClicks(false, false) auf jedes Kind gezwungen,
			// damit der Group-Drag auch im Lock-Mode greift. Das hat den Lock-
			// Mode zerstoert: Slider wurden nicht mehr fuer die User-Bedienung
			// freigegeben (Klicks fielen durch zur Group). Ohne dieses Override
			// verhalten sich die Children wieder standardmaessig nach
            // Module::setLocked (lock -> intercept, unlock -> pass through).
			// Trade-off: Layout-Edit der Children innerhalb des Panels
			// funktioniert nur im Unlock-Mode. Drag des Panels selbst geht
			// weiterhin per Title-Klick auch im Lock-Mode.
		}

		// Layout-Editierbarkeit dieser Group: folgt strikt dem globalen Lock.
		// Im Lock-Mode darf nichts mehr verschoben werden — Controls bleiben
		// bedienbar (Module::setLocked(true) -> intercept Klicks), aber Group-
		// und Child-Drag sind aus. Rechtsklick fuer das Z-Order-Menue bleibt
		// trotzdem moeglich (s. mouseDown — der pruefst isLayoutEditable nicht).
		bool isLayoutEditable() const {
			if (editor == nullptr) return false;
			return !editor->isLocked();
		}

		void mouseDown(const juce::MouseEvent& e) override {
			dragTarget = nullptr;

			constexpr int kTitleDragHeight = 25;

			// Rechtsklick zeigt das Z-Order-Kontextmenue — unabhaengig vom
			// Lock- oder Layout-Edit-State. Re-Stacking ist eine Layout-Aktion
			// die auch waehrend der Performance Sinn ergibt. Das Event kann
			// von einem Child stammen, wenn dieses im Lock-Mode den Klick
			// abfaengt und wir via addMouseListener als Listener angehaengt
			// sind — daher Position in Group-Koords zurueckrechnen und das
			// Hit-Module gleich aus dem Event-Component-Pfad ablesen.
			if (e.mods.isRightButtonDown()) {
				Module* hitChild = nullptr;
				if (e.eventComponent != this) {
					// Child hat den Klick abgefangen, wir bekommen ihn via
					// Listener. Den passenden Module*-Vorfahren bestimmen,
					// damit z.B. Klick auf den inneren Slider eines Knobs den
					// Knob selbst als Z-Order-Target liefert.
					juce::Component* c = e.eventComponent;
					while (c != nullptr && c != this) {
						if (auto* m = dynamic_cast<Module*>(c)) {
							hitChild = m;
							break;
						}
						c = c->getParentComponent();
					}
				} else {
					const auto pos = e.getPosition();
					for (int i = 0; i < getNumChildComponents(); ++i) {
						if (auto* m = dynamic_cast<Module*>(getChildComponent(i))) {
							if (m->getBounds().contains(pos)) { hitChild = m; break; }
						}
					}
				}
				if (hitChild != nullptr)
					showChildContextMenu(hitChild);
				else
					showGroupContextMenu();
				return;
			}

			// Linksklick auf einen Child von woanders (z.B. ein gelockter Knob
			// dispatcht via Listener an uns weiter): nicht behandeln — sonst
			// wuerde der Group-Drag jeden Knob-Klick im Lock-Mode kapern.
			if (e.eventComponent != this) return;

			// Linksklick-Drag braucht Layout-Edit-Modus (Unlock, oder
			// Instrument-Panel mit Override).
			if (!isLayoutEditable()) return;

			const auto pos = e.getPosition();
			Module* hitChild = nullptr;
			// Titel-Drag-Zone: Klicks in die oberen kTitleDragHeight Pixel
			// greifen IMMER die Group selbst, auch wenn dort ein Child-Bounds
			// reinragt. Sonst hat ein dicht gefuelltes Sub-Panel keinen
			// "leeren" Greifpunkt.
			if (pos.y >= kTitleDragHeight) {
				for (int i = 0; i < getNumChildComponents(); ++i) {
					if (auto* m = dynamic_cast<Module*>(getChildComponent(i))) {
						if (m->getBounds().contains(pos)) { hitChild = m; break; }
					}
				}
			}
			dragTarget = (hitChild != nullptr) ? static_cast<juce::Component*>(hitChild)
			                                   : static_cast<juce::Component*>(this);
			dragger.startDraggingComponent(dragTarget, e);
		}

		// Z-Order-Kontextmenue fuer ein surfaced Child. Hoeherer zIndex = weiter
		// vorne. Wir mirrorn die Aenderung gleich nach JUCE (toFront / toBack /
		// toBehind), sodass das Re-Stacking sofort sichtbar wird, und schreiben
		// den Wert auf das Module zurueck, damit er beim naechsten Save / Layer-
		// Toggle wieder aufgebaut wird.
		void showChildContextMenu(Module* child) {
			if (child == nullptr) return;

			// Defer via MessageManager — der Rechtsklick kommt via
			// MouseListener-Pfad waehrend Knob/Slider noch im eigenen
			// mouseDown stecken. Ein synchrones m.show() hier kann mit deren
			// eigenen Popups kollidieren und unsichtbar bleiben. callAsync
			// laesst den aktuellen Event-Dispatch erst zu Ende laufen.
			auto safeSelf  = juce::Component::SafePointer<RootLevelGroup>(this);
			juce::Component::SafePointer<Module> safeChild(child);
			juce::MessageManager::callAsync([safeSelf, safeChild]() {
				if (safeSelf == nullptr || safeChild == nullptr) return;

				juce::PopupMenu m;
				m.addItem(1, "Bring to Front");
				m.addItem(2, "Bring Forward");
				m.addItem(3, "Send Backward");
				m.addItem(4, "Send to Back");

				const int result = m.show();
				if (result > 0 && safeSelf != nullptr && safeChild != nullptr)
					safeSelf->applyZOrderAction(safeChild.getComponent(), result);
			});
		}

		// Kontextmenue fuer den leeren Bereich der Group: Hintergrundfarbe des
		// Panels setzen oder zuruecksetzen. Deferred via MessageManager analog
		// showChildContextMenu, da der Rechtsklick aus einem Listener-Pfad kommt.
		void showGroupContextMenu() {
			auto safeSelf = juce::Component::SafePointer<RootLevelGroup>(this);
			juce::MessageManager::callAsync([safeSelf]() {
				if (safeSelf == nullptr) return;

				const bool hasColour = safeSelf->sourceModule != nullptr
					&& safeSelf->sourceModule->hasSurfaceBackgroundColour();

				juce::PopupMenu m;
				m.addItem(1, "Set background color...");
				m.addItem(2, "Clear background color", hasColour);

				const int result = m.show();
				if (safeSelf == nullptr) return;
				if (result == 1) {
					safeSelf->pickBackgroundColour();
				}
				else if (result == 2 && safeSelf->sourceModule != nullptr) {
					safeSelf->sourceModule->clearSurfaceBackgroundColour();
					safeSelf->repaint();
					Project::getInstance()->setDirty(true);
				}
			});
		}

		// Open a ColourSelector in a CallOutBox anchored at the group. Live
		// preview via changeListenerCallback writes each change straight onto
		// the source module and repaints.
		void pickBackgroundColour() {
			auto selector = std::make_unique<juce::ColourSelector>(
				juce::ColourSelector::showColourAtTop
				| juce::ColourSelector::showSliders
				| juce::ColourSelector::showColourspace);
			selector->setName("Background");
			selector->setSize(300, 380);

			const juce::Colour initial =
				(sourceModule != nullptr && sourceModule->hasSurfaceBackgroundColour())
					? sourceModule->getSurfaceBackgroundColour()
					: juce::Colour(0xff222222);
			selector->setCurrentColour(initial, juce::dontSendNotification);
			selector->addChangeListener(this);
			// SafePointer auf den Selector behalten: wird die Group zerstoert
			// (z.B. Layer-Toggle leert rootLevelGroups), waehrend der CallOutBox
			// noch offen ist, deregistriert der Destruktor uns als Listener und
			// verhindert ein Feuern auf das tote Objekt.
			activeSelector = selector.get();

			juce::CallOutBox::launchAsynchronously(
				std::move(selector), getScreenBounds(), nullptr);
		}

		void changeListenerCallback(juce::ChangeBroadcaster* source) override {
			if (auto* cs = dynamic_cast<juce::ColourSelector*>(source)) {
				if (sourceModule != nullptr) {
					sourceModule->setSurfaceBackgroundColour(cs->getCurrentColour());
					repaint();
					Project::getInstance()->setDirty(true);
				}
			}
		}

		void applyZOrderAction(Module* child, int action) {
			if (child == nullptr) return;

			// Aktuelle Module-Children einsammeln, sortiert nach gespeichertem
			// zIndex. Luecken sind erlaubt (z.B. nach Laden); wir normalisieren
			// am Ende auf 0..N-1.
			std::vector<Module*> surfaced;
			for (int i = 0; i < getNumChildComponents(); ++i)
				if (auto* m = dynamic_cast<Module*>(getChildComponent(i)))
					surfaced.push_back(m);

			std::stable_sort(surfaced.begin(), surfaced.end(),
				[](Module* a, Module* b) {
					return (a != nullptr ? a->getZIndex() : 0)
					     < (b != nullptr ? b->getZIndex() : 0);
				});

			auto it = std::find(surfaced.begin(), surfaced.end(), child);
			if (it == surfaced.end()) return;
			const int idx = (int) std::distance(surfaced.begin(), it);
			const int lastIdx = (int) surfaced.size() - 1;

			int newIdx = idx;
			switch (action) {
				case 1: newIdx = lastIdx; break;      // Bring to Front
				case 2: newIdx = juce::jmin(lastIdx, idx + 1); break; // Forward
				case 3: newIdx = juce::jmax(0, idx - 1); break;       // Backward
				case 4: newIdx = 0; break;            // Send to Back
				default: return;
			}
			if (newIdx == idx) return;

			surfaced.erase(it);
			surfaced.insert(surfaced.begin() + newIdx, child);

			// JUCE-Z-Order direkt setzen: surfaced[0] erst ganz nach hinten,
			// dann jedes weitere darueber (toFront bringt es vor das vorige).
			// Frueher hatten wir hier addAndMakeVisible(child, i) — das ist
			// in dieser JUCE-Version ein no-op fuer schon eingehaengte Kinder
			// und das Re-Stacking blieb unsichtbar.
			if (!surfaced.empty()) {
				surfaced.front()->toBack();
				for (size_t i = 1; i < surfaced.size(); ++i)
					surfaced[i]->toFront(false);
			}

			// Resizer wieder ganz nach oben (resized() macht das sonst nur
			// beim naechsten Layout-Event).
			if (resizer != nullptr)
				resizer->toFront(false);

			// zIndex auf das Modul zurueckschreiben (0..N-1) fuer Persistenz.
			for (int i = 0; i < (int) surfaced.size(); ++i)
				surfaced[i]->setZIndex(i);

			repaint();
		}

		void mouseDrag(const juce::MouseEvent& e) override {
			if (dragTarget == nullptr) return;

			// Linked-Drag-Trigger:
			//  - Hauptpanel des Instruments (sourceModule selbst isInstrument):
			//    immer linked. Es ist DIE Performance-Einheit, ein Solo-Drag
			//    wuerde es vom Rest seines Instruments entkoppeln.
			//  - Sub-Group-Panels: default solo (Layout innerhalb des Instr.
			//    anpassen). Mit gehaltenem Cmd (auf macOS) bzw. Ctrl (Win/Linux)
			//    schalten wir auf linked um — so kann der User auch von einem
			//    Sub-Panel aus die ganze Anordnung verschieben, ohne das
			//    Hauptpanel zu suchen.
			const bool isMain = (sourceModule != nullptr && sourceModule->isInstrument());
			const bool linkedRequested = e.mods.isCommandDown();
			const bool moveAsInstrument =
				(dragTarget == this
				 && instrumentAncestor != nullptr
				 && (isMain || linkedRequested));

			const juce::Point<int> before = moveAsInstrument
				? getPosition()
				: juce::Point<int>();

			dragger.dragComponent(dragTarget, e, nullptr);

			if (moveAsInstrument) {
				const juce::Point<int> delta = getPosition() - before;
				if ((delta.x != 0 || delta.y != 0) && editor != nullptr)
					editor->moveInstrumentGroupSiblings(this, delta);
			}
		}

		void mouseUp(const juce::MouseEvent& e) override {
			if (dragTarget == nullptr) return;

			if (dragTarget == this && sourceModule != nullptr) {
				sourceModule->setSurfacePos(getPosition());
				// Geschwister-Persistenz spiegelt das mouseDrag-Trigger-Schema:
				// nur die Faelle, in denen wir die Geschwister tatsaechlich
				// mitbewegt haben, schreiben deren neue Positionen zurueck.
				const bool isMain = sourceModule->isInstrument();
				const bool linkedRequested = e.mods.isCommandDown();
				if (instrumentAncestor != nullptr && (isMain || linkedRequested)
					&& editor != nullptr)
					editor->persistInstrumentGroupSiblings(this);
			}
			else if (auto* m = dynamic_cast<Module*>(dragTarget)) {
				m->setSurfacePos(m->getPosition());
			}
			dragTarget = nullptr;
		}

	private:
		Module* sourceModule = nullptr;
		SynthEditor* editor = nullptr;
		// Naechster Modul-Ancestor mit isInstrument()==true; nullptr, wenn diese
		// Group keinem Instrument untergeordnet ist. Steuert isLayoutEditable
		// und den Geschwister-Drag (alle Groups eines Instruments wandern
		// zusammen).
		Module* instrumentAncestor = nullptr;
		juce::Component* dragTarget = nullptr;
		juce::ComponentDragger dragger;
		std::unique_ptr<juce::ResizableCornerComponent> resizer;
		juce::ComponentBoundsConstrainer constrainer;
		// Offener ColourSelector (in einer CallOutBox), bei dem wir als
		// ChangeListener registriert sind. Auto-null sobald er zerstoert wird.
		juce::Component::SafePointer<juce::ColourSelector> activeSelector;
	};
}

void SynthEditor::updateRootLevelGroupEditMode() {
	// Show/hide the corner resizer on every surfaced RootLevelGroup so the
	// user can only drag the group's size while in edit mode (lock == false).
	for (int i = 0; i < rootLevelGroups.size(); ++i) {
		if (auto* group = dynamic_cast<RootLevelGroup*>(rootLevelGroups[i])) {
			group->applyEditMode();
		}
	}
}

void SynthEditor::moveInstrumentGroupSiblings(juce::Component* leader, juce::Point<int> delta) {
	auto* lead = dynamic_cast<RootLevelGroup*>(leader);
	if (lead == nullptr) return;
	Module* instr = lead->getInstrumentAncestor();
	if (instr == nullptr) return;

	for (int i = 0; i < rootLevelGroups.size(); ++i) {
		auto* sibling = dynamic_cast<RootLevelGroup*>(rootLevelGroups[i]);
		if (sibling == nullptr || sibling == lead) continue;
		if (sibling->getInstrumentAncestor() != instr) continue;
		sibling->setTopLeftPosition(sibling->getPosition() + delta);
	}
}

void SynthEditor::persistInstrumentGroupSiblings(juce::Component* leader) {
	auto* lead = dynamic_cast<RootLevelGroup*>(leader);
	if (lead == nullptr) return;
	Module* instr = lead->getInstrumentAncestor();
	if (instr == nullptr) return;

	for (int i = 0; i < rootLevelGroups.size(); ++i) {
		auto* sibling = dynamic_cast<RootLevelGroup*>(rootLevelGroups[i]);
		if (sibling == nullptr || sibling == lead) continue;
		if (sibling->getInstrumentAncestor() != instr) continue;
		if (auto* src = sibling->getSourceModule())
			src->setSurfacePos(sibling->getPosition());
	}
}

void SynthEditor::collectRootLevelControls(Module* container) {
	// 1. Walk the whole subtree and bucket flagged controls by their immediate
	//    parent module. We skip controls that are already direct children of the
	//    editor root — those don't need surfacing.
	std::map<Module*, std::vector<Module*>> bucket;
	// Pro Bucket-Source der naechste Instrument-Ancestor (oder nullptr, wenn
	// keiner) — speist getInstrumentAncestor() der erzeugten RootLevelGroup
	// und damit den synchronen Geschwister-Drag.
	std::map<Module*, Module*> bucketInstrument;

	std::function<void(Module*, Module*)> walk = [&](Module* c, Module* instrAncestor) {
		Module* currentInstr = (c != nullptr && c->isInstrument()) ? c : instrAncestor;
		for (auto* m : *c->getModules()) {
			if (m->getLayer() == Module::Layer::GUI
				&& m->getDisplayAtRootLevel()
				&& c != root.get()) {
				bucket[c].push_back(m);
				bucketInstrument[c] = currentInstr;
			}
			if (!m->getModules()->empty())
				walk(m, currentInstr);
		}
	};
	walk(container, nullptr);

	// 2. For each source module with surfaceable controls, build a group, lay
	//    the controls out in a row inside it, and stack the groups vertically.
	//    User-saved positions (surfacePos) override the default layout for both
	//    groups and individual controls.
	const int defaultGroupX = 10;
	int defaultGroupY = 10;
	const int controlGap = 5;
	const int titleHeight = 25;
	const int padding = 10;

	for (auto& entry : bucket) {
		Module* source = entry.first;
		auto& controls = entry.second;

		// Z-Reihenfolge: niedriger zIndex zuerst einhaengen, damit der
		// hoechste zuletzt addAndMakeVisible aufgerufen wird und in JUCE's
		// Child-Liste oben landet. Stable, damit Controls mit gleichem zIndex
		// ihre relative Reihenfolge aus dem Modul-Vektor behalten.
		std::stable_sort(controls.begin(), controls.end(),
			[](Module* a, Module* b) {
				return (a != nullptr ? a->getZIndex() : 0)
				     < (b != nullptr ? b->getZIndex() : 0);
			});

		auto* group = new RootLevelGroup(source->getName(), source, this);
		group->setInstrumentAncestor(bucketInstrument[source]);

		int autoX = padding;
		int contentRight = padding;
		int contentBottom = titleHeight;

		for (auto* m : controls) {
			RootLevelProxy proxy;
			proxy.module = m;
			proxy.originalParent = m->getParentComponent();
			proxy.originalPosition = m->getPosition();
			proxy.originalLayer = m->getCurrentLayer();

			// Render at GUI size and let the user actually operate the control.
			m->setCurrentLayer(Module::Layer::GUI);
			m->setLocked(this->isLocked());

			juce::Point<int> childPos;
			if (m->hasSurfacePos()) {
				childPos = m->getSurfacePos();
			}
			else {
				childPos = juce::Point<int>(autoX, titleHeight);
				autoX += m->getWidth() + controlGap;
			}
			m->setTopLeftPosition(childPos.x, childPos.y);
			group->addAndMakeVisible(m);
			// Group als Listener anhaengen — damit das Z-Order-Kontextmenue
			// per Rechtsklick auch ueber einem gelockten Child auftaucht
			// (Child fangt den Klick sonst ab, group->mouseDown sieht ihn nicht).
			// wantsEventsForAllNestedChildComponents=true, weil ein Knob seinen
			// internen Slider als Klick-Empfaenger hat.
			m->addMouseListener(group, true);

			contentRight = std::max(contentRight, childPos.x + m->getWidth());
			contentBottom = std::max(contentBottom, childPos.y + m->getHeight());

			rootLevelProxies.push_back(proxy);
		}

		const int autoW = std::max(contentRight + padding, 150);
		const int autoH = contentBottom + padding;

		juce::Point<int> groupPos = source->hasSurfacePos()
			? source->getSurfacePos()
			: juce::Point<int>(defaultGroupX, defaultGroupY);

		// User-resized? Honour the saved size, but never shrink below the
		// content's auto-fit so dragged-out controls don't get clipped on
		// the next layer toggle.
		const int groupW = source->hasSurfaceSize()
			? std::max(source->getSurfaceSize().getX(), autoW)
			: autoW;
		const int groupH = source->hasSurfaceSize()
			? std::max(source->getSurfaceSize().getY(), autoH)
			: autoH;

		group->setBounds(groupPos.x, groupPos.y, groupW, groupH);
		if (!source->hasSurfacePos())
			defaultGroupY += groupH + 10;

		addAndMakeVisible(group);
		rootLevelGroups.add(group);

		// applyEditMode() lief schon im RootLevelGroup-Ctor, da hatten die
		// Children aber noch keinen Parent. Fuer Instrument-Panels muss das
		// Click-Intercept-Override nochmal ueber die jetzt eingehaengten
		// Children laufen, damit der Layout-Drag auch beim ersten Surfacing
		// greift.
		group->applyEditMode();
	}
}

void SynthEditor::restoreRootLevelControls() {
	for (auto& proxy : rootLevelProxies) {
		if (proxy.module == nullptr) continue;

		proxy.module->setCurrentLayer(proxy.originalLayer);
		proxy.module->setTopLeftPosition(proxy.originalPosition.x, proxy.originalPosition.y);

		// Group-MouseListener wieder ablaedeln (in collectRootLevelControls
		// per addMouseListener fuer das Z-Order-Kontextmenue gesetzt). Sonst
		// haengt nach dem naechsten rootLevelGroups.clear() ein dangling
		// Listener am Modul und der naechste Klick crasht.
		if (auto* host = proxy.module->getParentComponent())
			proxy.module->removeMouseListener(host);

		if (proxy.originalParent != nullptr) {
			proxy.originalParent->addAndMakeVisible(proxy.module);
		}
		else if (auto* p = proxy.module->getParentComponent()) {
			// Module wasn't attached to any parent before — detach from the
			// group so it doesn't get destroyed alongside it.
			p->removeChildComponent(proxy.module);
		}
	}
	rootLevelProxies.clear();
	rootLevelGroups.clear();
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
	if (key.getKeyCode() == 65 && isLeftShiftDown) { // Shift-A opens add menu
		showPrefabMenu(Point<int>(mouseX, mouseY));
	}
	else if (key.getKeyCode() == 65 && isCtrlDown) {
		for (int i = 0; i < root->getModules()->size(); i++) {
			root->getModules()->at(i)->setSelected(true);
			selectionModel.getSelectedModules().push_back(root->getModules()->at(i));
		}
	}
	// Sub-Editor (openEditor registriert ihn als eigenen KeyListener auf sich
	// selbst) verschluckt sonst Delete, bevor MainComponent::keyPressed feuert.
	// Folge: weder Module noch Connections im Sub-Modul-Tab lassen sich per
	// Tastatur loeschen. Toolbar-Delete trifft die Selection auch nicht, weil
	// die Mausbewegung zum Toolbar mouseMove->checkForConnectionSelect ausloest
	// und das c->selected aller nicht-ueberfahrenen Connections zurueck-resettet
	// — Module ueberleben, Connections nicht.
	else if (key.getKeyCode() == KeyPress::deleteKey
	      || key.getKeyCode() == KeyPress::backspaceKey) {
		deleteSelectionWithCascade(this, navigator);
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
	// Listener (ControlSurfacePanel u.a.) ueber den bevorstehenden Teardown
	// informieren — VOR jeder Modul-Loeschung, damit sie Bindings/Shadow-
	// Klone freigeben koennen, solange die Source-Slider-ValueSources noch
	// gueltig sind.
	for (size_t i = 0; i < listener.size(); ++i)
		if (listener.at(i) != nullptr)
			listener.at(i)->beforeCleanUp();

	// Undo-History GANZ AM ANFANG leeren — bevor wir Module aus dem root-Tree
	// loeschen. RemoveSelectedAction & Co. besitzen die "removed" Module
	// (damit Undo sie zurueckholen kann) und deleten sie in ihrem Destruktor.
	// Wenn wir die Module zuerst aus root loeschen und dann clearUndoHistory
	// aufrufen, sehen die Actions in ~RemoveSelectedAction freed Pointer und
	// crashen mit double-free / UAF (z.B. ~DonutModule auf weg-deletetem
	// Modul). Reihenfolge umkehren beseitigt beide Pfade — Actions raeumen
	// ihre eigenen Module sauber ab, der nachfolgende Modul-Loop sieht nur
	// noch live Module.
	if (auto* p = Project::getInstance())
		if (auto* um = p->getUndoManager())
			um->clearUndoHistory();

	if (navigator != nullptr)
		navigator->clearTracks();

	// ---- Tab-Cleanup ZUERST, vor jeder Modul-Loeschung. ----
	// Tab-Inhalte (Sub-Editoren, Sample-/Recorder-/Sequencer-Editoren) sind
	// teils mit deleteOnRemove=false angemeldet und gehoeren in Wirklichkeit
	// den Modulen, die wir gleich loeschen. Wuerden wir die Module zuerst
	// killen, dispatchen Timer/Listener der Tabs noch in den freigegebenen
	// Speicher; und der spaetere removeTab() faende einen toten Component
	// vor (set parent=null an Dangling-Pointer = UAF).

	// Sub-Editor-Tabs (per "Edit Group") detachen und freigeben — `openViews`
	// haelt die Eigentums-Refs auf die Viewports, die Tab-Refs sind
	// deleteOnRemove=false.
	if (tab != nullptr) {
		for (auto* v : openViews) {
			if (v == nullptr) continue;
			for (int i = tab->getNumTabs(); --i >= 0;) {
				if (tab->getTabContentComponent(i) == v) {
					tab->removeTab(i);
					break;
				}
			}
		}
	}
	for (auto* v : openViews)
		delete v;
	openViews.clear();

	// Verbleibende extra Tabs im Haupt-TabbedComponent: nur die erste Lasche
	// (der Editor selbst) bleibt; alles andere kommt weg.
	if (tab != nullptr) {
		while (tab->getNumTabs() > 1)
			tab->removeTab(tab->getNumTabs() - 1);
		tab->setCurrentTabIndex(0);
	}

	// Supplemental-Tabs (Sample editor, Audio recorder, Step sequencer,
	// Event editor — oft Modul-eigene Components mit deleteOnRemove=false).
	// Pianoroll-Tab wurde schon in navigator->clearTracks() abgeraeumt.
	// Project kann beim App-Quit schon zerstoert sein — getInstance liefert
	// dann nullptr und wir ueberspringen den Block.
	if (auto* proj = Project::getInstance()) {
		if (proj->getAppMode() == Project::AppMode::STUDIO) {
			if (auto* sup = proj->getSupplemental()) {
				while (sup->getNumTabs() > 1)
					sup->removeTab(sup->getNumTabs() - 1);
				sup->setCurrentTabIndex(0);
			}
		}
	}

	// MasterChannelPanel haelt einen Raw-Pointer auf eine VolumeAdjustable
	// (Master-AudioOut oder Track-AudioOut/AuxOut). Slider-Listener feuern
	// via AsyncUpdater — bereits gequeue'te Slider-Events koennen erst NACH
	// cleanUp aus der Message-Queue dispatchen und dann auf das schon
	// freigegebene Modul derefen (vtable-UAF in sliderValueChanged → setVolume).
	// Pointer auf ALLEN Channel-Strips nullen — Track-Strips UND Master —
	// BEVOR irgendwas geloescht wird. Der Null-Check am Anfang von
	// sliderValueChanged greift dann statt des Crashs.
	if (mixer != nullptr) {
		for (auto* panel : mixer->getChannels()) {
			if (panel != nullptr) panel->setModule(nullptr);
		}
		if (auto* master = mixer->getMasterPanel()) {
			master->setModule(nullptr);
		}
	}

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

	// rootLevelProxies/rootLevelGroups halten rohe Module*/Component*-Pointer,
	// die wir oben mit den Modulen geloescht haben. Beim naechsten openFile()
	// laeuft am Ende setCurrentLayer(savedLayer) — restoreRootLevelControls()
	// derefiert dann proxy.module und kracht (UAF). rootLevelGroups ist eine
	// OwnedArray und loescht die GroupComponents korrekt mit; den Vector der
	// Proxies kippen wir vorsorglich auch.
	rootLevelProxies.clear();
	rootLevelGroups.clear();

	// Drag-/Selection-State auf Modul-Pointer resetten BEVOR root weggeht —
	// startPin zeigt sonst weiter auf einen Pin in einem geloeschten Module,
	// und selectionModel haelt root als rohen Pointer (setRoot(nullptr) hier
	// vermeidet dangling deref im naechsten select/clearSelection-Aufruf).
	startPin = nullptr;
	selectionModel.getSelectedModules().clear();
	selectionModel.setRoot(nullptr);

	// PropertyView haelt selectedTrack/selectedModule als rohe Pointer auf
	// Tracks/Module aus Projekt A. Mit selectionChanged(nullptr) leeren wir
	// den Panel und nullen selectedTrack — sonst kann ein Klick auf den
	// "Show Plugin"-Button nach dem Reload in einen freigegebenen Track
	// dispatchen.
	if (auto* proj = Project::getInstance())
		if (auto* p = proj->getPropertyView())
			p->selectionChanged(nullptr);

	root = nullptr;
	mixer->clearChannels();
	// clearUndoHistory wird ganz am Anfang von cleanUp aufgerufen; ein
	// zweiter Aufruf hier waere harmlos (Stack ist schon leer), aber wir
	// sparen ihn uns aus Klarheit.
	removeAllChangeListeners();

	// Sanity-Checks: wer hier knallt, hat einen neuen rohen Pointer auf
	// Project-A-State eingebaut, ohne ihn beim Tear-Down freizugeben. Im
	// Release-Build sind das No-Ops; im Debug fliegen sie fruehzeitig auf,
	// statt erst beim naechsten Klick als BAD_ACCESS zu kommen.
	jassert(root == nullptr);
	jassert(startPin == nullptr);
	jassert(selectionModel.getSelectedModules().empty());
	jassert(rootLevelProxies.empty());
	jassert(rootLevelGroups.isEmpty());
	jassert(openViews.empty());
}

bool SynthEditor::confirmDiscardIfDirty() {
	if (!isDirty()) return true;
	return juce::AlertWindow::showOkCancelBox(
		juce::AlertWindow::AlertIconType::NoIcon, "Hey!",
		"You have unsaved resources, wanna start from scratch anyway?");
}

void SynthEditor::newFile(bool askIfDirty) {

	setRunning(false);
	bool makeNewFile = true;

	if (askIfDirty && isDirty()) {
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

	if (xml == nullptr) {
		juce::Logger::writeToLog("loadFromString: XML parse failed — aborting.");
		return;
	}

	ValueTree v = ValueTree::fromXml(*xml.get());

	setRunning(false);
	ModuleUtils::loadStructure(root->getModules(), root->getConnections(), &v, this);

	// Walk the full tree (incl. nested submodules) when seeding
	// PrefabFactory::lastModuleIndex. Old root-only scan let nested indices
	// stay above lastModuleIndex unnoticed, so newly-created prefabs ended
	// up colliding with sub-group indices (see radiogroup.slb: two Audio
	// Selectors share index="19" — second one's connections vanish on reload
	// because loadConnections finds the first by index first).
	PrefabFactory::getInstance()->updateLastModuleIndex(
		ModuleUtils::maxIndexInTree(root->getModules()));

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

	// Master FX live outside the module graph and are persisted as a
	// separate ValueTree child. Apply them here.
	if (mixer != nullptr && mixer->getMasterPanel() != nullptr) {
		ValueTree masterFX = v.getChildWithName("MasterFXChain");
		if (masterFX.isValid())
			mixer->getMasterPanel()->applyFXChainState(masterFX);
	}

	// Bus-FX-Chains analog wieder einspielen.
	if (mixer != nullptr) {
		ValueTree busFX = v.getChildWithName("BusFXChains");
		if (busFX.isValid())
			mixer->applyBusFXChains(busFX);
	}

	setCurrentLayer(v.getProperty("layer").toString().getIntValue());
	setLocked(v.getProperty("lock").toString().getIntValue() > 0);

	notifyListeners();

	xml = nullptr;
	resized();

	setRunning(true);
}

void SynthEditor::saveModule(Module* m) {

	// Pre-select ~/.Synthlab/modules/ and suggest "<moduleName>.slb" so the
	// saved file lands where the Add menu's "User Modules" submenu scans.
	const File modulesDir = getUserModulesDir();
	const String suggestedName = m != nullptr && m->getName().isNotEmpty()
								   ? m->getName() + ".slb"
								   : String("module.slb");
	const File initial = modulesDir.getChildFile(suggestedName);

	FileChooser chooser("Select target file...", initial, "*.slb");

	if (chooser.browseForFileToSave(true)) {

		ValueTree* v = new ValueTree("Module");
		v->setProperty("name", m->getName(), nullptr);
		v->setProperty("isInstrument", m->isInstrument(), nullptr);

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

		if (xml == nullptr) {
			juce::Logger::writeToLog("loadModule: XML parse failed for "
				+ file.getFullPathName());
			juce::AlertWindow::showMessageBoxAsync(
				juce::AlertWindow::WarningIcon,
				"Could not open module",
				"The selected file could not be parsed as a module.");
			return nullptr;
		}

		Project::getInstance()->addRecentFile(file.getFullPathName());
		ValueTree v = ValueTree::fromXml(*xml.get());

		setRunning(false);
		// Block the audio thread — see openFile(File) for the full rationale.
		const juce::ScopedLock graphBarrier(
			AudioManager::getInstance()->getProcessGraphLock());

		Module* module = ModuleUtils::loadFromXml(v, this);
		module->setName(v.getProperty("name"));
		module->setTopLeftPosition(mouseX, mouseY);

		this->addAndMakeVisible(module);
		this->getModule()->getModules()->push_back(module);

		// Loading a .slb drops modules with potentially very high saved indices
		// (e.g. millisecond-counter shifts from saveModule's updateIndices) into
		// the current project. Without bumping PrefabFactory::lastModuleIndex
		// past every loaded index — INCLUDING nested submodules — the next
		// prefab created reuses a low index that collides with one inside the
		// freshly-loaded group, and the group's internal connections start
		// silently breaking on save/reload.
		{
			long localMax = module->getIndex();
			const long childMax = ModuleUtils::maxIndexInTree(module->getModules());
			if (childMax > localMax) localMax = childMax;
			PrefabFactory::getInstance()->updateLastModuleIndex(localMax);
		}

		// Heal index collisions inside the loaded .slb (see openFile for the
		// detailed rationale — same code path, two non-prefab groups can both
		// arrive at index=0 from a legacy save).
		{
			auto collisions = ModuleUtils::findIndexCollisions(module->getModules());
			for (auto& kv : collisions) {
				juce::String names;
				for (auto& n : kv.second) names << n << " ";
				Logger::writeToLog("WARN: .slb load collision idx="
					+ juce::String(kv.first) + " shared by: " + names
					+ "— auto-renumbering duplicate.");
			}
			if (ModuleUtils::autoFixIndexCollisions(module->getModules()) > 0)
				setDirty(true);
		}

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


void SynthEditor::saveFileAs() {
	// Always show the file chooser regardless of the project's current path.
	// Implemented as a one-shot toggle of the "new file" flag so we can reuse
	// every code path inside saveFile() (master-FX persistence, _tracks.xml /
	// _audio.xml siblings, dirty reset, updateProject) without duplicating it.
	const bool wasNew = Project::getInstance()->isNewFile();
	Project::getInstance()->setNewFile(true);
	saveFile();
	// saveFile() flips newFile back to false on success; if the user cancelled
	// the chooser, restore the previous state so a stale flag doesn't surprise
	// the next plain Save.
	if (Project::getInstance()->isNewFile())
		Project::getInstance()->setNewFile(wasNew);
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

	// Master-channel FX live outside the module graph and aren't covered by
	// saveStructure; persist them here so they reappear on load.
	if (mixer != nullptr && mixer->getMasterPanel() != nullptr) {
		ValueTree masterFX = mixer->getMasterPanel()->getFXChainState();
		if (masterFX.isValid() && masterFX.getNumChildren() > 0)
			v->appendChild(masterFX, nullptr);
	}

	// Bus-FX-Chains pro BusOut. Selber Grund wie Master-FX: leben ausserhalb
	// des Modul-Graphen. Aufloesung beim Load ueber busModuleIndex —
	// Module::getIndex() ist Save/Load-stabil.
	if (mixer != nullptr) {
		ValueTree busFX = mixer->collectBusFXChains();
		if (busFX.isValid() && busFX.getNumChildren() > 0)
			v->appendChild(busFX, nullptr);
	}

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

	// Master-channel FX live outside the module graph and aren't covered by
	// saveStructure; persist them here so they reappear on load.
	if (mixer != nullptr && mixer->getMasterPanel() != nullptr) {
		ValueTree masterFX = mixer->getMasterPanel()->getFXChainState();
		if (masterFX.isValid() && masterFX.getNumChildren() > 0)
			v->appendChild(masterFX, nullptr);
	}

	// Bus-FX-Chains pro BusOut. Selber Grund wie Master-FX: leben ausserhalb
	// des Modul-Graphen. Aufloesung beim Load ueber busModuleIndex —
	// Module::getIndex() ist Save/Load-stabil.
	if (mixer != nullptr) {
		ValueTree busFX = mixer->collectBusFXChains();
		if (busFX.isValid() && busFX.getNumChildren() > 0)
			v->appendChild(busFX, nullptr);
	}

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
		// Edit-Persist: AudioRegions mit veraendertem Buffer (Normalize,
		// Fade, Reverse, Consolidate ...) als neue WAV nach $projectDir/edits/
		// schreiben und ihre clipRefIds aktualisieren. Muss VOR
		// navigator->getConfiguration() laufen, damit das Tracks-XML schon
		// die neuen refIds enthaelt und Project::save() den Pfad ins
		// _audio.xml schreibt.
		if (navigator != nullptr) {
			const juce::File editsDir = file.getParentDirectory().getChildFile("edits");
			navigator->persistDirtyAudioRegions(editsDir);
		}

		std::unique_ptr<XmlElement> xml = v->createXml();
		xml->writeToFile(file, "");
		xml = nullptr;
		delete v;

		if (navigator != nullptr) {
			ValueTree tracks = navigator->getConfiguration();

			// FX-Slot-Map pro Track anreichern: Channel-Strip zu jedem Track
			// suchen und dessen User-Slot-Zuordnung ans Track-XML haengen.
			// Ohne das verlieren wir nach reload die Position der FX im Strip.
			if (mixer != nullptr) {
				const auto& navTracks = navigator->getTracks();
				const int numTracks = juce::jmin((int) navTracks.size(), tracks.getNumChildren());
				for (int ti = 0; ti < numTracks; ++ti) {
					Track* t = navTracks.at((size_t) ti);
					if (t == nullptr) continue;
					for (auto* panel : mixer->getChannels()) {
						if (panel == nullptr) continue;
						if (panel->getTrack() == t) {
							auto trackChild = tracks.getChild(ti);
							if (trackChild.isValid())
								trackChild.appendChild(panel->getFXSlotMap(), nullptr);
							break;
						}
					}
				}
			}

			xml = tracks.createXml();
			// juce::File::getChildFile baut plattformkorrekt — vorher war hier
			// "\\" hardcoded und auf macOS landete die Datei nie wo sie sollte.
			xml->writeToFile(file.getParentDirectory().getChildFile(file.getFileNameWithoutExtension() + "_tracks.xml"), "");
			xml = nullptr;
		}

		setDirty(false);
		Project::getInstance()->save(file.getParentDirectory().getChildFile(file.getFileNameWithoutExtension() + "_audio.xml"));
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

	// Der SynthEditor(double,int)-Ctor setzt Project::setRoot(seinen frischen
	// "Root"), und setModuleRef() unten gibt genau dieses Modul gleich wieder
	// frei — ohne Bewahrung bliebe Project::getRoot() ein dangling Pointer auf
	// das freigegebene Sub-Editor-Root. Der echte Projekt-Root ist und bleibt
	// der des Haupt-Editors; daher hier sichern und nach setModuleRef
	// wiederherstellen. (TrackNavigator iteriert Project::getRoot() auf dem
	// Audio-Thread und crashte sonst beim Play, sobald ein Sub-Editor offen war.)
	Module* projectRoot = Project::getInstance()->getRoot();

	SynthEditor* editor = new SynthEditor(_sampleRate, bufferSize);
	Viewport* editorView = new Viewport();
	editorView->setSize(500, 200);
	editorView->setViewedComponent(editor);
	editorView->setScrollBarsShown(true, true);
	editorView->setScrollOnDragEnabled(false);
	editorView->setWantsKeyboardFocus(false);
	editorView->setMouseClickGrabsKeyboardFocus(false);
	editor->setModuleRef(m);
	// Project::root zuruecksetzen — der Ctor oben hat ihn auf den jetzt
	// freigegebenen Sub-Editor-Root umgebogen.
	Project::getInstance()->setRoot(projectRoot);
	editor->setIndex(static_cast<int>(m->getIndex()));
	editor->setTab(tab);
	editor->addEditorListener(Project::getInstance()->getPropertyView());
	// SynthEditor implementiert KeyListener (zwei-Argument keyPressed). Damit
	// modifierKeysChanged + keyPressed im Sub-Editor ueberhaupt feuern (Shift-A
	// fuer Add-Menue, Cmd-A fuer Select-All), muss der Editor als KeyListener
	// auf sich selbst registriert sein. Selbe Logik beim Root-Editor laeuft
	// in MainComponent::MainComponent (Suche nach "editor->addKeyListener(editor)").
	editor->addKeyListener(editor);
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
#if JUCE_IOS
		if (!confirmDiscardIfDirty()) return;
		cleanUp();
		newFile(/*askIfDirty=*/ false);

		URL url = chooser.getURLResult();
		InputStream* is = url.createInputStream(false);
		if (is == nullptr) {
			juce::Logger::writeToLog("openFile (iOS): could not open input stream for "
				+ url.toString(false));
			return;
		}
		String data = is->readEntireStreamAsString();
		delete is;

		ScopedPointer<XmlElement> xml = XmlDocument(data).getDocumentElement();

		if (xml == nullptr) {
			juce::Logger::writeToLog("openFile (iOS): XML parse failed.");
			juce::AlertWindow::showMessageBoxAsync(
				juce::AlertWindow::WarningIcon,
				"Could not open project",
				"The selected file is not a valid Synthlab project.");
			return;
		}

		ValueTree v = ValueTree::fromXml(*xml.get());
		setRunning(false);
		// Block the audio thread — see openFile(File) for the full rationale.
		const juce::ScopedLock graphBarrier(
			AudioManager::getInstance()->getProcessGraphLock());
		ModuleUtils::loadStructure(root->getModules(), root->getConnections(), &v, this);

		PrefabFactory::getInstance()->updateLastModuleIndex(
			ModuleUtils::maxIndexInTree(root->getModules()));

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

		if (mixer != nullptr && mixer->getMasterPanel() != nullptr) {
			ValueTree masterFX = v.getChildWithName("MasterFXChain");
			if (masterFX.isValid())
				mixer->getMasterPanel()->applyFXChainState(masterFX);
		}

		// Bus-FX-Chains nach reconstructAllFXChains anwenden — zu dem Zeitpunkt
		// sind die BusOut-Module und ihre Panels bereits registriert.
		if (mixer != nullptr) {
			ValueTree busFX = v.getChildWithName("BusFXChains");
			if (busFX.isValid())
				mixer->applyBusFXChains(busFX);
		}

		Project::getInstance()->setRoot(root.get());
		resized();
		setCurrentLayer(v.getProperty("layer").toString().getIntValue());
		setLocked(v.getProperty("lock").toString().getIntValue() > 0);
		notifyListeners();
		xml = nullptr;

		updateProject(url);
		Project::getInstance()->setNewFile(false);
		setRunning(true);
#else
		// Desktop path delegates to the File overload so "Open Recent" runs the
		// exact same restore sequence as the file chooser.
		openFile(chooser.getResult());
#endif
	}
}

#if !JUCE_IOS
void SynthEditor::openFile(juce::File file) {
	if (!file.existsAsFile())
		return;

	// Same crash class as File > New > Cancel: cleanUp() runs before any
	// dialog had a chance to bail out. Confirm BEFORE any teardown.
	if (!confirmDiscardIfDirty()) return;

	// Audio-Thread VOR cleanUp anhalten — sonst iteriert er bei jedem Wechsel
	// ueber bereits-geloeschte Module/Tracks und es kracht entweder im Audio-
	// Pfad oder per ChangeBroadcaster, sobald Mixer broadcastet (z.B. bei
	// Mixer::addTrack waehrend des Loads von File B).
	setRunning(false);

	// Block the audio thread for the entire load. SynthEditor::setRunning
	// alone is unreliable (idempotent on this->running, returns early at
	// app start without entering any lock — the audio callback then races
	// with loadStructure's push_backs). The dedicated processGraphLock is
	// independent of running-flag state — the audio thread try-locks it in
	// MainComponent::getNextAudioBlock and emits silence if held.
	const juce::ScopedLock graphBarrier(
		AudioManager::getInstance()->getProcessGraphLock());

	cleanUp();
	newFile(/*askIfDirty=*/ false);

	// Defektes Projektfile darf den App-Prozess nicht reissen. Der Crash
	// ueber Loader -> Plugin-State-Restore -> NSApp-Rethrow ist bisher der
	// Hauptweg, ueber den eine kaputte .slb die App killt; alles ab hier
	// laeuft daher in einem try/catch, der im Fehlerfall den Editor in den
	// "frisches, leeres Projekt"-Zustand zurueckdreht und einen Dialog
	// zeigt, statt durchzucrashen.
	// Muss aussen stehen, damit der catch-Handler den Progress-Dialog
	// schliessen kann, wenn der Load mittendrin wirft.
	juce::Component::SafePointer<juce::DialogWindow> progressWindow;

	auto dismissProgress = [&progressWindow]() {
		if (auto* dw = progressWindow.getComponent())
			dw->exitModalState(0);
	};

	auto reportLoadFailure = [&file](const juce::String& detail) {
		juce::Logger::writeToLog("openFile: load failed for "
			+ file.getFullPathName() + " — " + detail);
		juce::AlertWindow::showMessageBoxAsync(
			juce::AlertWindow::WarningIcon,
			"Could not open project",
			"The file\n" + file.getFullPathName()
				+ "\ncould not be loaded:\n" + detail
				+ "\n\nStarting with an empty project.");
	};

	try {

	std::unique_ptr<XmlElement> xml = XmlDocument(file).getDocumentElement();
	if (xml == nullptr) {
		reportLoadFailure("file is not a valid Synthlab project (XML parse failed).");
		newFile(/*askIfDirty=*/ false);
		setRunning(true);
		return;
	}

	Project::getInstance()->addRecentFile(file.getFullPathName());

	ValueTree v = ValueTree::fromXml(*xml.get());

	// Progress dialog. Project load runs synchronously on the message thread
	// (plugin APIs require it) under the audio graph lock, so no general
	// message pump is safe — broadcaster listeners must not observe the tree
	// mid-build. ProjectLoadProgressDialog::setProgress flushes only paints
	// via ComponentPeer::performAnyPendingRepaintsNow.
	const int totalModules = ModuleUtils::countModulesInValueTree(v);
	auto* progressDialog = new ProjectLoadProgressDialog();
	juce::DialogWindow::LaunchOptions progressOpts;
	progressOpts.dialogTitle = "Projekt wird geladen...";
	progressOpts.dialogBackgroundColour = juce::Colours::black;
	progressOpts.useNativeTitleBar = true;
	progressOpts.resizable = false;
	progressOpts.escapeKeyTriggersCloseButton = false;
	progressOpts.content.setOwned(progressDialog);
	progressOpts.content->setSize(400, 80);
	progressWindow = progressOpts.launchAsync();
	juce::Component::SafePointer<ProjectLoadProgressDialog> progressSafe = progressDialog;
	if (progressSafe)
		progressSafe->setProgress(0.0);

	int modulesDone = 0;
	auto onModuleLoaded = [&modulesDone, totalModules, progressSafe]() {
		++modulesDone;
		if (auto* d = progressSafe.getComponent())
			d->setProgress(totalModules > 0 ? (double)modulesDone / (double)totalModules : 1.0);
	};

	ModuleUtils::loadStructure(root->getModules(), root->getConnections(), &v, this, onModuleLoaded);

	// Walk the full tree (incl. nested submodules) when seeding
	// PrefabFactory::lastModuleIndex. Old root-only scan let nested indices
	// stay above lastModuleIndex unnoticed, so newly-created prefabs ended
	// up colliding with sub-group indices (see radiogroup.slb: two Audio
	// Selectors share index="19" — second one's connections vanish on reload
	// because loadConnections finds the first by index first).
	PrefabFactory::getInstance()->updateLastModuleIndex(
		ModuleUtils::maxIndexInTree(root->getModules()));

	// Heal legacy files that came in with colliding module indices (one of the
	// historic causes of "connections vanish on reload" — a duplicate module
	// shadowed the original in loadConnections' linear find-by-index). The
	// renumber walks the tree, keeps the first occurrence of each index, and
	// assigns a fresh index from PrefabFactory to every later duplicate;
	// terminal pairings inside the renumbered module are re-pointed alongside.
	// Runtime connections survive (they hold Pin*/Module*), the next save
	// then writes a clean, internally consistent file.
	{
		auto collisions = ModuleUtils::findIndexCollisions(root->getModules());
		for (auto& kv : collisions) {
			juce::String names;
			for (auto& n : kv.second) names << n << " ";
			Logger::writeToLog("WARN: module index collision idx=" + juce::String(kv.first)
				+ " shared by: " + names + "— auto-renumbering duplicate.");
		}
		const int fixed = ModuleUtils::autoFixIndexCollisions(root->getModules());
		if (fixed > 0) {
			setDirty(true);
			Logger::writeToLog("autoFixIndexCollisions: " + juce::String(fixed)
				+ " module(s) renumbered — file is now flagged dirty, save to persist.");
		}
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

	// Master FX live outside the module graph (see MasterChannelPanel::insertFX)
	// and are persisted as a separate ValueTree child. Apply them here.
	if (mixer != nullptr && mixer->getMasterPanel() != nullptr) {
		ValueTree masterFX = v.getChildWithName("MasterFXChain");
		if (masterFX.isValid())
			mixer->getMasterPanel()->applyFXChainState(masterFX);
	}

	// Bus-FX-Chains analog.
	if (mixer != nullptr) {
		ValueTree busFX = v.getChildWithName("BusFXChains");
		if (busFX.isValid())
			mixer->applyBusFXChains(busFX);
	}

	Project::getInstance()->setRoot(root.get());

	resized();

	setCurrentLayer(v.getProperty("layer").toString().getIntValue());
	setLocked(v.getProperty("lock").toString().getIntValue() > 0);

	notifyListeners();

	xml = nullptr;

	openTracks(file);

	updateProject(file);
	// Mark the project as "has a known file" so the next Save writes back to
	// this path instead of popping a Save-As dialog.
	Project::getInstance()->setNewFile(false);

	if (progressSafe)
		progressSafe->setProgress(1.0);
	if (auto* dw = progressWindow.getComponent())
		dw->exitModalState(0);

	setRunning(true);
	return;

	} catch (const std::exception& e) {
		// loadStructure / plugin restore / region constructors koennen werfen;
		// Editor in einen sauberen Zustand zurueckdrehen, damit die App weiter
		// nutzbar bleibt. Progress-Dialog vorher wegklicken — sonst bleibt er
		// halbvoll stehen, blockiert die UI und wirkt wie ein Hang.
		dismissProgress();
		try { cleanUp(); } catch (...) {}
		try { newFile(/*askIfDirty=*/ false); } catch (...) {}
		setRunning(true);
		reportLoadFailure(juce::String(e.what()));
	} catch (...) {
		dismissProgress();
		try { cleanUp(); } catch (...) {}
		try { newFile(/*askIfDirty=*/ false); } catch (...) {}
		setRunning(true);
		reportLoadFailure("unknown error during project load.");
	}
}

bool SynthEditor::saveFileTo(const juce::File& target)
{
	if (target == juce::File()) return false;

	// saveFile() opens a chooser only when isNewFile() is true; otherwise it
	// writes to currentFilePath. Redirect it by flipping both into the
	// "non-new, has-path" state.
	//
	// saveFile()'s non-chooser branch additionally guards on file.exists() -
	// it only overwrites existing files, never creates fresh ones. Touch the
	// target first so the guard engages. juce::File::create also creates any
	// missing parent directories.
	target.create();

	auto* p = Project::getInstance();
	p->setCurrentFilePath(target.getFullPathName());
	p->setNewFile(false);
	saveFile();
	return target.existsAsFile();
}

bool SynthEditor::openFileNoPrompt(const juce::File& source)
{
	if (!source.existsAsFile()) return false;
	// confirmDiscardIfDirty() (called inside openFile(File)) is a no-op when
	// the project isn't dirty — clear the flag preemptively so no modal
	// pops up on the message thread mid-script.
	setDirty(false);
	openFile(source);
	return true;
}
#else
void SynthEditor::openFile(juce::File /*file*/) {
	// On iOS files are accessed through URLs/InputStreams; the no-arg openFile()
	// handles that path inline.
}

bool SynthEditor::saveFileTo(const juce::File& /*target*/) { return false; }
bool SynthEditor::openFileNoPrompt(const juce::File& /*source*/) { return false; }
#endif

namespace {
	// BFS durch das Event-Pin-Graph: Folge ab `startOutPin` allen Event-
	// Verbindungen vorwaerts, bis ein PluginModule gefunden wird. Module
	// zwischen TrackIn und Plugin (MIDI-Thru, Channel-Filter, MidiProcessor,
	// usw.) werden als Pass-Through behandelt: deren OUT-EVENT-Pins werden
	// weiterverfolgt.
	//
	// Hintergrund: der frueher hier eingebaute direkte Match (TrackIn.pin5
	// → erste Connection → muss direkt auf PluginModule.pin liegen) hat in
	// Projekten mit Zwischenmodulen versagt. Folge: Track::pluginModule
	// blieb nach Load nullptr, Bounce-in-Place und Track-Bounce schweigen.
	PluginModule* findPluginModuleDownstream(Pin* startOutPin, Module* root)
	{
		if (startOutPin == nullptr || root == nullptr || root->getModules() == nullptr)
			return nullptr;

		// Pin → Module Map einmal aufbauen, damit der Owner-Lookup im Loop
		// nicht jedes Mal alle Module abklappert.
		std::map<Pin*, Module*> pinToModule;
		for (Module* m : *root->getModules()) {
			if (m == nullptr) continue;
			for (Pin* p : m->getPins())
				if (p != nullptr) pinToModule[p] = m;
		}

		std::vector<Pin*> queue;
		std::set<Pin*> visitedPins;
		std::set<Module*> visitedModules;

		for (Pin* c : startOutPin->getConnections())
			if (c != nullptr) queue.push_back(c);

		while (!queue.empty()) {
			Pin* p = queue.back();
			queue.pop_back();
			if (!visitedPins.insert(p).second) continue;

			auto it = pinToModule.find(p);
			if (it == pinToModule.end()) continue;
			Module* m = it->second;
			if (!visitedModules.insert(m).second) continue;

			if (auto* pm = dynamic_cast<PluginModule*>(m)) return pm;

			// Pass-Through: alle OUT-EVENT-Pins dieses Moduls weiterverfolgen.
			for (Pin* outP : m->getPins()) {
				if (outP == nullptr) continue;
				if (outP->direction != Terminal::Direction::OUT) continue;
				if (outP->getType()  != Terminal::Type::EVENT) continue;
				for (Pin* c : outP->getConnections())
					if (c != nullptr) queue.push_back(c);
			}
		}

		return nullptr;
	}
}

void SynthEditor::openTracks(File file)
{

	// Plattformkorrekt: getChildFile statt String-Konkatenation mit "\\".
	File parent = file.getParentDirectory();
	File audioDataFile = parent.getChildFile(file.getFileNameWithoutExtension() + "_audio.xml");
	File trackDataFile = parent.getChildFile(file.getFileNameWithoutExtension() + "_tracks.xml");

	std::unique_ptr<XmlElement> audioXml = XmlDocument(audioDataFile).getDocumentElement();

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
		String path = audio.getChild(i).getProperty("path");
		String id   = audio.getChild(i).getProperty("refId");

		Project::getInstance()->addAudioFile(id, path);
	}

	std::unique_ptr<XmlElement> xml = XmlDocument(trackDataFile).getDocumentElement();

	// Tracks-XML kann fehlen oder kaputt sein, ohne dass das eigentliche
	// Projekt-XML defekt war. Bisheriger Deref crashte hart — jetzt
	// laden wir das Modul-Graph trotzdem, Tracks bleiben halt leer.
	if (xml == nullptr) {
		juce::Logger::writeToLog("openTracks: tracks XML missing or unparsable at "
			+ trackDataFile.getFullPathName() + " — skipping track restore.");
		return;
	}

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
		// Bound nicht mehr v.getNumChildren() — seit Timeline-Marker als
		// "Markers"-Sibling neben den Tracks im Tracks-XML stehen kann der
		// Loop sonst Marker als Track interpretieren und tracks.at(i)
		// out-of-range gehen.
		const int numTracks = juce::jmin((int) tracks.size(), v.getNumChildren());
		for (int i = 0; i < numTracks; i++) {

			ValueTree track = v.getChild(i);
			if (track.getType().toString() != "Track") break;
			TrackIn* ti = tracks.at(i);

			Track* loadedTrack = nullptr;
			if (track.getProperty("type") == "audio") {
				Track* t = navigator->addTrack(Track::Type::AUDIO, Project::getInstance()->getSampleRate());
				ti->setTrack(t);
				t->setName(track.getProperty("name"));

				ValueTree regions = track.getChild(0);

				for (int j = 0; j < regions.getNumChildren(); j++) {
					ValueTree region = regions.getChild(j);
					const String clipRefId = region.getProperty("clipRefId").toString();
					// Skip orphaned regions: empty clipRefId means there is no
					// audio file to load — SmartSample::loadFromFile would throw
					// std::runtime_error on the empty path and propagate up
					// through Cocoa's event dispatch, killing the app.
					if (clipRefId.isEmpty())
						continue;
					Logger::getCurrentLogger()->writeToLog(clipRefId);
					Region* r = t->addRegion(clipRefId, File(Project::getInstance()->getAudioPath(clipRefId)), Project::getInstance()->getSampleRate(), region.getProperty("sampleOffset").toString().getIntValue());

					// Trim aus dem gespeicherten numSamples ableiten, nicht aus
					// dem Pixel-`width`. width ist nur am gespeicherten Zoom
					// gueltig und macht aus konsolidierten/duplizierten Regions
					// nach Reload wieder Voll-Laenge-Regionen, weil addRegion()
					// frisch mit dem ungetrimmten Original-File belegt — der
					// Trim muss explizit zurueckgespielt werden.
					const long savedNumSamples = (long)(juce::int64) region.getProperty("numSamples", (juce::int64) 0);
					const double sr = Project::getInstance()->getSampleRate();
					const float zoom = t->getZoom();
					int targetWidth = 0;
					if (savedNumSamples > 0 && zoom > 0.0f && sr > 0.0)
						targetWidth = (int) std::round((double) savedNumSamples / sr * zoom);
					else
						targetWidth = (int) region.getProperty("width", 0);

					if (auto* ar = dynamic_cast<AudioRegion*>(r)) {
						ar->setSourceStartSample((long)(juce::int64) region.getProperty("sourceStartSample", (juce::int64) 0));
						// Persistierte Trim-Laenge uebernehmen, damit ein
						// nachfolgendes Save den exakten Wert zurueckschreibt
						// (statt erneut Pixel->Samples zu runden).
						if (savedNumSamples > 0)
							ar->setTrimmedNumSamples(savedNumSamples);

						// Tempo-Follow Status wiederherstellen. Property fehlt in
						// alten Projekten -> Default-Werte aus dem Konstruktor
						// bleiben aktiv (referenceTempo = current project tempo,
						// followTempo = false).
						const juce::var refTempoVar = region.getProperty("referenceTempo");
						if (! refTempoVar.isVoid())
							ar->setReferenceTempo((double) refTempoVar);
						ar->setFollowTempo((bool) region.getProperty("followTempo", false));

						// Pitch-State (Halbtoene). Der geladene Buffer ist
						// bereits in diesem Zustand — wir setzen NUR das Member
						// direkt, kein setPitch (das wuerde nochmal shiften).
						const juce::var pitchVar = region.getProperty("pitch");
						if (! pitchVar.isVoid()) {
							// Hack: pitch ist private mit nur setPitch als Setter,
							// und setPitch shiftet. Wir gehen ueber einen Delta-
							// Trick: Member-Wert ist aktuell 0, neuer Save-Wert
							// soll ohne Shift uebernommen werden. Dafuer haben
							// wir aber keinen direkten Setter — addieren wir
							// einen klein-invasiven adoptPitchState() bei
							// Bedarf. Vorlaeufig: setPitch macht bei kleiner
							// Delta (< 0.5 semitones, siehe Impl) eh nur das
							// Property-Sync. Wir setzen erst Member via
							// adoptLoadedPitch.
							ar->adoptLoadedPitch((float) (double) pitchVar);
						}

						// Per-region gain (linear). Property absent in legacy
						// projects -> default 1.0 from the AudioRegion member
						// remains in effect.
						const juce::var gainVar = region.getProperty("gain");
						if (! gainVar.isVoid())
							ar->setGain((float) (double) gainVar);
					}

					// Trim mit unterdruecktem componentMovedOrResized-Handler
					// setzen — der Snap/Clamp-Pfad in der wasResized-Branch
					// laesst sonst eine soeben geladene Nachbar-Region sich an
					// einer abutting Region "vorbeisnappen" und macht den Trim
					// rueckgaengig. Wir trust dem gespeicherten Stand: setSize
					// direkt, dann freigeben.
					if (targetWidth > 0 && targetWidth != r->getWidth()) {
						r->setBoundsUpdateInProgress(true);
						r->setSize(targetWidth, r->getHeight());
						r->setBoundsUpdateInProgress(false);
					}

					// Load-Time-Stretch: wenn followTempo und Tempo-Mismatch,
					// jetzt mit Rubberband auf das aktuelle Projekt-Tempo
					// ziehen. applyTempoStretch markiert die Region als
					// mutated, der naechste Save schreibt den gestretchten
					// Buffer in edits/ raus.
					if (auto* ar = dynamic_cast<AudioRegion*>(r)) {
						if (ar->getFollowTempo()) {
							const double curTempo = (double) Project::getInstance()->getTempo();
							const double sr = Project::getInstance()->getSampleRate();
							if (std::abs(ar->getReferenceTempo() - curTempo) > 1e-4)
								ar->applyTempoStretch(curTempo, sr);
						}
					}
				}
				loadedTrack = t;
			}
			else if (track.getProperty("type") == "midi") {
				juce::Point<int> pos = juce::Point<int>(10, 10);
				Track* t = navigator->addTrack(Track::Type::MIDI, Project::getInstance()->getSampleRate());
				ti->setTrack(t);
				t->setName(track.getProperty("name"));
				t->setMidiChannel(track.getProperty("midiChannel").toString().getIntValue(), false);
				t->setMidiOutputDevice(track.getProperty("interface").toString());
				loadMidiRegions(track, t);
				loadedTrack = t;
			}
			else if (track.getProperty("type") == "instrument") {
				juce::Point<int> pos = juce::Point<int>(10, 10);
				Track* t = navigator->addTrack(Track::Type::INSTRUMENT, Project::getInstance()->getSampleRate());
				ti->setTrack(t);
				t->setName(track.getProperty("name"));
				t->setMidiChannel(track.getProperty("midiChannel").toString().getIntValue(), false);

				// Re-link the PluginModule to the track.
				// TrackIn.pin5 (event-out) → BFS durch das Event-Pin-Graph →
				// erster PluginModule downstream. Toleriert beliebig viele
				// Pass-Through-Module zwischen TrackIn und dem Synth-Plugin.
				auto& tiPins = ti->getPins();
				if (tiPins.size() > 5) {
					if (auto* pm = findPluginModuleDownstream(tiPins.at(5), getModule()))
						t->setPluginModule(pm);
				}

				loadMidiRegions(track, t);
				loadedTrack = t;
			}

			// Automation pro Track laden — gleiche Stelle, an der getConfiguration()
			// das ValueTree als "Automation"-Child geschrieben hat.
			if (loadedTrack != nullptr) {

				// Track-Namen final aus dem Save uebernehmen. addRegion()/
				// addMidiRegion() setzen bei der ERSTEN Region den Track-Namen
				// auf den Region-Namen (Default-Naming beim Clip-Drop) und
				// ueberschreiben damit den weiter oben gesetzten geladenen
				// Namen — daher hier nach dem Region-Load erneut setzen.
				const juce::String savedTrackName = track.getProperty("name").toString();
				if (savedTrackName.isNotEmpty())
					loadedTrack->setName(savedTrackName);

                Logger::getCurrentLogger()->writeToLog("Loading track "+loadedTrack->getName());

				ValueTree autoTree = track.getChildWithName("Automation");
				if (autoTree.isValid())
					loadedTrack->getAutomationModel().fromValueTree(autoTree);

				// UI-Zustand der Bus-Automation wiederherstellen. setBusAutomationTarget
				// bindet das Overlay an die oben bereits geladene Lane (findOrCreate
				// findet sie). syncFromTrack() im TrackControl uebernimmt danach
				// Toggle-Status und Dropdown-Auswahl beim Navigator-Layout.
				const long busAutoBus = (long) (juce::int64) track.getProperty("busAutoBus", (juce::int64) -1);
				if (busAutoBus >= 0) {
					const int busAutoParam = (int) track.getProperty("busAutoParam", 0);
					loadedTrack->setBusAutomationTarget(busAutoBus, (Track::BusParam) busAutoParam);
				}
				loadedTrack->setBusAutomationVisible((bool) track.getProperty("busAutoVisible", false));

				// Bus-Sends pro Track laden. Bus-Existenz wird nicht geprueft:
				// der BusOut wird beim Load des Patch-XML separat ueber
				// configureAudioModule registriert; wenn der referenzierte
				// busModuleIndex spaeter im Audio-Loop nicht auffindbar ist,
				// wird der Send einfach uebersprungen (kein Lookup-Fehler).
				ValueTree sendsTree = track.getChildWithName("Sends");
				if (sendsTree.isValid()) {
					for (int s = 0; s < sendsTree.getNumChildren(); ++s) {
						const auto sendNode = sendsTree.getChild(s);
						if (! sendNode.hasType("Send")) continue;
						const long busIdx = (long) (juce::int64) sendNode.getProperty("busModuleIndex", (juce::int64) -1);
						const float amt   = (float)              sendNode.getProperty("amount",         1.0);
						if (busIdx >= 0)
							loadedTrack->addSend(busIdx, amt);
					}
				}
			}
		}

		// Re-creating the Navigator tab and bringing it to the front is handled
		// by MainComponent right after editor->openFile() returns. We can't do it
		// here because the Viewport (`trackView`) that wraps the navigator is
		// owned by EditorComponent, and SynthEditor::cleanUp() — invoked at the
		// start of openFile() — already removed the Navigator tab from topTab.
	}

	// Track <-> MasterChannelPanel re-linken. Beim NEU-Anlegen eines Tracks
	// macht das TrackUtil::AddInstrumentTrack() inline; im Lade-Pfad fehlt es
	// bisher und damit weiss kein Channel-Strip, zu welchem Track er gehoert.
	// Folge: Track::fxPlugins[] wird nie befuellt und FX-Plugin-Automation
	// spielt nicht ab. Match: panel->chainSourceMod == TrackIn dieses Tracks,
	// fallback ueber fxChainContains(track->pluginModule).
	//
	// Anschliessend pro Track die gespeicherte FX-Slot-Map auf das Panel
	// anwenden — sonst rutschen alle Plugins nach reconstructFXChainFromGraph
	// linksbuendig zusammen und das Instrument landet als fxChain[0].
	if (mixer != nullptr) {
		// Map TrackIn -> ValueTree (das geladene Track-Element) aufbauen,
		// damit wir spaeter die FXSlots-Property finden koennen.
		std::map<TrackIn*, juce::ValueTree> tiToConfig;
		// v ist hier der oben geladene tracks-Tree.
		for (int i = 0; i < v.getNumChildren(); ++i) {
			juce::ValueTree trackCfg = v.getChild(i);
			// Reihenfolge in tracks.xml entspricht der TrackIn-Reihenfolge im Modul-Graph.
			int j = 0;
			for (auto* mod : *getModule()->getModules()) {
				auto* ti = dynamic_cast<TrackIn*>(mod);
				if (ti == nullptr) continue;
				if (j == i) { tiToConfig[ti] = trackCfg; break; }
				++j;
			}
		}

		for (auto* mod : *getModule()->getModules()) {
			auto* ti = dynamic_cast<TrackIn*>(mod);
			if (ti == nullptr) continue;
			Track* t = ti->getTrack();
			if (t == nullptr) continue;
			for (auto* panel : mixer->getChannels()) {
				if (panel == nullptr) continue;
				if (panel->isFedBy(ti) || panel->fxChainContains(t->getPluginModule())) {
					panel->setTrack(t); // ruft syncTrackFXPlugins() -> Cache rebuild
					// FX-Slot-Map aus dem geladenen Track-XML anwenden, falls vorhanden.
					auto cfgIt = tiToConfig.find(ti);
					if (cfgIt != tiToConfig.end()) {
						auto slotMap = cfgIt->second.getChildWithName("FXSlots");
						if (slotMap.isValid())
							panel->applyFXSlotMap(slotMap);
					}
					break;
				}
			}
		}
	}

	// Timeline-Marker laden — sitzen als "Markers"-Sibling neben den Track-
	// Children. Wird zuletzt gemacht, damit das TimeLine-Component dann
	// bereits korrekt initialisiert ist (TrackNavigator existiert bereits).
	if (navigator != nullptr) {
		auto markersTree = v.getChildWithName("Markers");
		if (markersTree.isValid()) {
			auto* tl = navigator->getTimeLine();
			if (tl != nullptr) {
				tl->clearMarkers();
				for (int i = 0; i < markersTree.getNumChildren(); ++i) {
					auto mk = markersTree.getChild(i);
					TimelineMarker tm;
					tm.position    = (double) mk.getProperty("position", 0.0);
					tm.name        = mk.getProperty("name", "Marker").toString();
					tm.type        = (mk.getProperty("type", "normal").toString() == "loop")
						               ? TimelineMarker::Type::Loop
						               : TimelineMarker::Type::Normal;
					tm.loopRepeats = juce::jmax(1, (int) mk.getProperty("loopRepeats", 1));
					tl->addMarker(tm);
				}
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

		// Buffer-Offset wiederherstellen, sonst springen Noten beim Reload
		// einer links-getrimmten Region zurueck zur ungetrimmten Position.
		mr->setSourceStartSample((long)(juce::int64) region.getProperty("sourceStartSample", (juce::int64) 0));

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
				const juce::int64 timestamp = (juce::int64) event.getProperty("timestamp");
				// Saved timestamp is BUFFER-relativ — d.h. enthaelt bereits
				// sourceStartSample (wenn die Region links getrimmt war). Den
				// raw-Pfad nehmen, der den Wert 1:1 setzt; addMessage() wuerde
				// sourceStartSample ein zweites Mal addieren und die Notes nach
				// dem Reload sichtbar verschieben (und im konsolidierten Save
				// hinter den rechten Rand schieben, sodass sie verschwinden).
				mr->addMessageRaw(new juce::MidiMessage(msg), timestamp);
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

	// Den vom User vergebenen Channel-Namen (von ModuleUtils::loadStructure
	// auf das Modul gestasht) auf den frisch angelegten Strip + Channel
	// zurueckschreiben. Wird nach addChannel aufgerufen, weil dort erst der
	// Default-Name (z.B. vom Audio-Device) gesetzt wurde.
	auto applySavedChannelName = [this](VolumeAdjustable* va, int channelIndex) {
		if (va == nullptr) return;
		const juce::String saved = va->getSavedChannelName();
		if (saved.isEmpty()) return;
		for (auto* panel : mixer->getChannels())
			if (panel != nullptr && panel->getIndex() == channelIndex) {
				panel->setName(saved);
				break;
			}
		for (auto* ch : Mixer::getInstance()->getChannels())
			if (ch != nullptr && ch->index == channelIndex) {
				ch->name = saved;
				break;
			}
	};

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
		applySavedChannelName(out, channelIndex);
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
			applySavedChannelName(in, channelIndex);
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
		applySavedChannelName(aux, channelIndex);
	}

	BusOut* bus;

	if ((bus = dynamic_cast<BusOut*>(m)) != NULL) {
		mixer->getBusChannels().push_back(bus);
		String channelName = "Bus " + String(mixer->getBusChannels().size());
		int channelIndex = addChannel(channelName, Mixer::Channel::Type::BUS);
		bus->setChannelIndex(channelIndex);
		mixer->setVolume(channelIndex, bus->getVolume());
		mixer->setModule(channelIndex, bus);
		applySavedChannelName(bus, channelIndex);
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

void SynthEditor::deleteSelectionWithCascade(SynthEditor* editor, TrackNavigator* navigator)
{
    if (editor == nullptr) return;

    auto* um = Project::getInstance()->getUndoManager();
    um->beginNewTransaction();

    // 1) Selected TrackIn -> cascade into TrackDeleteAction for its Track.
    //    Pre-collect first so we don't iterate while the editor's module
    //    vector shrinks under us.
    if (navigator != nullptr) {
        std::vector<TrackIn*> selectedTrackIns;
        for (auto* m : *editor->getModule()->getModules()) {
            if (m != nullptr && m->isSelected()) {
                if (auto* ti = dynamic_cast<TrackIn*>(m))
                    selectedTrackIns.push_back(ti);
            }
        }
        for (auto* ti : selectedTrackIns) {
            Track* t = ti->getTrack();
            if (t == nullptr) continue;
            // TrackDeleteAction will also collect this TrackIn as part of
            // the assembly — drop the selection so the follow-up
            // RemoveSelectedAction doesn't try to delete it a second time.
            ti->setSelected(false);
            auto* tda = new TrackDeleteAction(navigator, editor, t);
            um->perform(tda);
        }
    }

    // 2) Whatever's left in the selection (other modules, connections) goes
    //    through the regular path. perform() returns false on an empty
    //    selection and the UndoManager drops the action — no harm.
    auto* rsa = new RemoveSelectedAction(editor);
    um->perform(rsa);
}

void SynthEditor::addVirtualInstrumentTrack()
{
	PluginBrowserComponent::show([this](juce::String selectedPlugin)
	{
		addVirtualInstrumentTrack(selectedPlugin);
	});
}

void SynthEditor::addVirtualInstrumentTrack(juce::String pluginName)
{
	if (pluginName.isEmpty())
		return;

	// Mirror the same setup the plugin-browser path performs, but skip the
	// dialog because the caller (Instruments tab double-click) already chose.
	if (navigator == nullptr) {
		// No track view yet — addTrack would crash. The caller is expected to
		// have ensured the navigator exists; bail out quietly otherwise so a
		// stray double-click can't tank the app.
		return;
	}
	auto* action = new AddTrackAction(this, navigator,
	                                  AddTrackAction::Type::Instrument,
	                                  pluginName);
	Project::getInstance()->getUndoManager()->beginNewTransaction();
	Project::getInstance()->getUndoManager()->perform(action);
	repaint();
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

	// Seed the root module so per-block loops downstream (TerminalModule
	// AUDIO copy etc.) use the real device buffer size from frame zero
	// instead of the header default. processModule propagates these to
	// every child each buffer.
	//
	// Zusaetzlich rekursiv durch den kompletten Modul-Baum. Module::setBuffersize
	// recurst nicht von selbst (siehe Module.cpp:564) — gehosteten Plugins muss
	// die neue Block-Groesse aber explizit per prepareToPlay mitgeteilt werden,
	// sonst arbeitet jedes PluginModule weiter mit dem Wert vom Load-Zeitpunkt.
	if (root != nullptr) {
		root->setSampleRate((float)sampleRate);
		root->setBuffersize(samplesPerBlockExpected);

		std::function<void(Module*)> propagate = [&](Module* m) {
			if (m == nullptr) return;
			auto* children = m->getModules();
			if (children == nullptr) return;
			for (auto* c : *children) {
				if (c == nullptr) continue;
				c->setSampleRate((float)sampleRate);
				c->setBuffersize(samplesPerBlockExpected);
				propagate(c);
			}
		};
		propagate(root.get());
	}

	// Master-FX-Kette lebt ausserhalb des Modul-Graphs (siehe
	// MasterChannelPanel::insertFX) — separat durchreichen.
	if (mixer != nullptr) {
		if (auto* master = mixer->getMasterPanel()) {
			for (int i = 0; i < master->getNumFxSlots(); ++i) {
				if (auto* pm = master->getFxPlugin(i)) {
					pm->setSampleRate((float)sampleRate);
					pm->setBuffersize(samplesPerBlockExpected);
				}
			}
		}
	}

	Logger::writeToLog("SynthEditor using sample rate " + String(sampleRate) + " Hz and buffer size of " + String(samplesPerBlockExpected) + " samples.");

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

void SynthEditor::removeEditorListener(EditorListener* l) {
	this->listener.erase(std::remove(this->listener.begin(),
	                                 this->listener.end(),
	                                 l),
	                     this->listener.end());
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
	// tab kann zur Init-Zeit noch nullptr sein (MainComponent::setTab kommt
	// erst nach dem EditorComponent-Konstruktor — und z.B. das Anlegen von
	// Default-Tracks in OpenTrackView triggert setDirty schon davor).
	if (tab != nullptr) {
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
			if (!confirmDiscardIfDirty()) { delete f; return; }
			FileInputStream* fis = new FileInputStream(*f);
			String data = fis->readEntireStreamAsString();
			updateProject(*f);
			delete fis;
			delete f;
			setRunning(false);
			cleanUp();
			newFile(/*askIfDirty=*/ false);
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
	commands.add({ SynthEditor::CommandIds::SAVE_AS });
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
	case SynthEditor::CommandIds::SAVE_AS:
		result.setInfo("Save As...", String(), String(), 0);
		result.addDefaultKeypress('s', ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
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
		// Ask BEFORE tearing anything down -- see confirmDiscardIfDirty.
		if (!confirmDiscardIfDirty()) return true;
		setRunning(false);
		cleanUp();
		newFile(/*askIfDirty=*/ false);
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
	else if (info.commandID == SynthEditor::CommandIds::SAVE_AS) {
		saveFileAs();
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
		SynthEditor::deleteSelectionWithCascade(this, navigator);
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
