/*
  ==============================================================================

    FileButtonPropertyComponent.cpp
    Created: 15 May 2018 11:03:07am
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "FileButtonPropertyComponent.h"

FileButtonPropertyComponent::FileButtonPropertyComponent(String name, Value& valueToControl) : ButtonPropertyComponent(name,true), value(valueToControl) {
    this->buttonText = name;

}

FileButtonPropertyComponent::~FileButtonPropertyComponent() {
    
}

String FileButtonPropertyComponent::getButtonText() const {
    return buttonText;
}

void FileButtonPropertyComponent::buttonClicked() {
    FileChooser chooser("Select file to open", File::getSpecialLocation(File::userDocumentsDirectory), "*",true, true);
    
    if (chooser.browseForFileToOpen()) {
        File file = chooser.getResult();
        String path = file.getFullPathName();
        value.setValue(path);
    }
}
