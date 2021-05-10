/*
  ==============================================================================

    CloseableTabButton.cpp
    Created: 25 Apr 2018 5:51:34pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "CloseableTabButton.h"

using juce::String;
using juce::ImageButton;
using juce::ImageCache;
using juce::TabbedButtonBar;

CloseableTabButton::CloseableTabButton(const String& name, TabbedButtonBar& ownerBar): juce::TabBarButton(name, ownerBar) {
    ImageButton* ib = new ImageButton();

    ib->setImages (false,
               false,
               true,
               ImageCache::getFromMemory(BinaryData::close_png, BinaryData::close_pngSize),
               1.0,
               Colours::white,
               ImageCache::getFromMemory(BinaryData::close_png, BinaryData::close_pngSize),
               1.0,
               Colours::orange,
               ImageCache::getFromMemory(BinaryData::close_png, BinaryData::close_pngSize),
               1.0,
               Colours::orange);
    
    TabBarButton::setExtraComponent(ib, juce::TabBarButton::ExtraComponentPlacement::afterText);
}

CloseableTabButton::~CloseableTabButton() {
    
}
