/*
  ==============================================================================

    CloseableTabButton.cpp
    Created: 25 Apr 2018 5:51:34pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "CloseableTabButton.h"

CloseableTabButton::CloseableTabButton(const String& name, TabbedButtonBar& ownerBar): juce::TabBarButton(name, ownerBar) {
    TextButton* tb = new TextButton();
    tb->setButtonText("x");
    TabBarButton::setExtraComponent(tb, juce::TabBarButton::ExtraComponentPlacement::afterText);
}

CloseableTabButton::~CloseableTabButton() {
    
}
