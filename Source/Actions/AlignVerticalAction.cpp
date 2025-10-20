/*
  ==============================================================================

    AlignVerticalAction.cpp
    Created: 20 Oct 2025 7:57:58pm
    Author:  mpue

  ==============================================================================
*/

#include "AlignVerticalAction.h"
#include "../SynthEditor.h"

bool AlignVerticalAction::perform()
{
	SelectionModel& selectionModel = editor->getSelectionModel();

	if (selectionModel.getSelectedModules().size() >= 2) {

		int minX = editor->getWidth();
		int minY = editor->getHeight();

		int currentLayer = editor->getCurrentLayer();

		if (currentLayer == Module::Layer::ALL) {

			// find the topmost-topleft coordinate

			for (int i = 0; i < selectionModel.getSelectedModules().size(); i++) {
				if (selectionModel.getSelectedModules().at(i)->getPosition().getX() < minX) {
					minX = selectionModel.getSelectedModules().at(i)->getPosition().getX();
				}
				if (selectionModel.getSelectedModules().at(i)->getPosition().getY() < minY) {
					minY = selectionModel.getSelectedModules().at(i)->getPosition().getY();
				}
			}

			/// now position the modules in the desired raster
			for (int i = 0; i < selectionModel.getSelectedModules().size(); i++) {
				selectionModel.getSelectedModules().at(i)->savePosition();
				selectionModel.getSelectedModules().at(i)->setTopLeftPosition(minX, minY + i * selectionModel.getSelectedModules().at(i)->getHeight());

			}
		}
		else {
			// find the topmost-topleft coordinate of the UI positions

			for (int i = 0; i < selectionModel.getSelectedModules().size(); i++) {
				if (selectionModel.getSelectedModules().at(i)->getUiPosition().getX() < minX) {
					minX = selectionModel.getSelectedModules().at(i)->getUiPosition().getX();
				}
				if (selectionModel.getSelectedModules().at(i)->getPosition().getY() < minY) {
					minY = selectionModel.getSelectedModules().at(i)->getUiPosition().getY();
				}
			}

			/// now position the modules in the desired raster
			for (int i = 0; i < selectionModel.getSelectedModules().size(); i++) {
				selectionModel.getSelectedModules().at(i)->saveUiPosition();
				selectionModel.getSelectedModules().at(i)->setUiPosition(minX, minY + i * selectionModel.getSelectedModules().at(i)->getHeight());
				selectionModel.getSelectedModules().at(i)->setTopLeftPosition(selectionModel.getSelectedModules().at(i)->getUiPosition());
			}
		}

		return true;

	}
	return false;
}

bool AlignVerticalAction::undo()
{
	SelectionModel& selectionModel = editor->getSelectionModel();
	int currentLayer = editor->getCurrentLayer();

	if (currentLayer == Module::Layer::ALL) {
		for (int i = 0; i < selectionModel.getSelectedModules().size(); i++) {
			selectionModel.getSelectedModules().at(i)->setTopLeftPosition(selectionModel.getSelectedModules().at(i)->getSavedPosition());
		}
	}
	else {
		for (int i = 0; i < selectionModel.getSelectedModules().size(); i++) {			
			selectionModel.getSelectedModules().at(i)->setTopLeftPosition(selectionModel.getSelectedModules().at(i)->getSavedUiPosition());
		}
	}
	return true;
}
