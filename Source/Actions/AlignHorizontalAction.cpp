/*
  ==============================================================================

    AlignHorizontalAction.cpp
    Created: 20 Oct 2025 7:58:12pm
    Author:  mpue

  ==============================================================================
*/

#include "AlignHorizontalAction.h"
#include "../SynthEditor.h"

bool AlignHorizontalAction::perform()
{
	SelectionModel& selectionModel = editor->getSelectionModel();

	if (selectionModel.getSelectedModules().size() >= 2) {

		int minX = editor->getWidth();
		int minY = editor->getHeight();
		
		int currentLayer = editor->getCurrentLayer();

		if (currentLayer == Module::Layer::ALL) {

			// find the topmost-topleft coordinate for the GUI layer position

			for (int i = 0; i < selectionModel.getSelectedModules().size(); i++) {
				if (selectionModel.getSelectedModules().at(i)->getPosition().getX() < minX) {
					minX = selectionModel.getSelectedModules().at(i)->getPosition().getX();
				}
				if (selectionModel.getSelectedModules().at(i)->getPosition().getY() < minY) {
					minY = selectionModel.getSelectedModules().at(i)->getPosition().getY();
				}
			}

			for (int i = 0; i < selectionModel.getSelectedModules().size(); i++) {
				selectionModel.getSelectedModules().at(i)->savePosition();
				selectionModel.getSelectedModules().at(i)->setTopLeftPosition(minX + i * selectionModel.getSelectedModules().at(i)->getWidth(), minY);
			}
		}
		else {

			// find the topmost-topleft coordinate for the GUI layer position

			for (int i = 0; i < selectionModel.getSelectedModules().size(); i++) {
				if (selectionModel.getSelectedModules().at(i)->getUiPosition().getX() < minX) {
					minX = selectionModel.getSelectedModules().at(i)->getUiPosition().getX();
				}
				if (selectionModel.getSelectedModules().at(i)->getUiPosition().getY() < minY) {
					minY = selectionModel.getSelectedModules().at(i)->getUiPosition().getY();
				}
			}

			for (int i = 0; i < selectionModel.getSelectedModules().size(); i++) {
				selectionModel.getSelectedModules().at(i)->saveUiPosition();
				selectionModel.getSelectedModules().at(i)->setUiPosition(minX + i * selectionModel.getSelectedModules().at(i)->getWidth(), minY);
				selectionModel.getSelectedModules().at(i)->setTopLeftPosition(selectionModel.getSelectedModules().at(i)->getUiPosition());
			}
		}

		return true;
	}
	
	return false;
}

bool AlignHorizontalAction::undo()
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
