/*
  ==============================================================================

    ModernLookAndFeel.cpp
    Created: Modern adaptation of CustomLookAndFeel
    Author:  Based on Matthias Pueski's work

  ==============================================================================
*/

#include "ModernLookAndFeel.h"

using juce::Graphics;
using juce::Slider;
using juce::Time;
using juce::ImageCache;
using juce::Image;

// Modern button styling with subtle shadows and rounded corners
void ModernLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
    const juce::Colour& backgroundColour,
    bool shouldDrawButtonAsHighlighted,
    bool shouldDrawButtonAsDown) {
    auto bounds = button.getLocalBounds().toFloat();
    auto cornerSize = modernCornerRadius;

    // Shadow effect for depth
    if (!shouldDrawButtonAsDown) {
        juce::Path shadowPath;
        shadowPath.addRoundedRectangle(bounds.translated(0, shadowOffset), cornerSize);

        juce::DropShadow shadow(juce::Colour(0x30000000), shadowRadius, juce::Point<int>(0, shadowOffset));
        shadow.drawForPath(g, shadowPath);
    }

    // Button background with state-based coloring
    juce::Colour buttonColour = backgroundColour;

    if (shouldDrawButtonAsDown) {
        buttonColour = buttonColour.darker(0.1f);
        bounds = bounds.translated(0, 1); // Pressed effect
    }
    else if (shouldDrawButtonAsHighlighted) {
        buttonColour = buttonColour.brighter(0.05f);
    }

    // Gradient background for modern look
    juce::ColourGradient gradient(
        buttonColour.brighter(0.02f), bounds.getX(), bounds.getY(),
        buttonColour.darker(0.02f), bounds.getX(), bounds.getBottom(),
        false
    );

    g.setGradientFill(gradient);
    g.fillRoundedRectangle(bounds, cornerSize);

    // Subtle border
    g.setColour(buttonColour.contrasting(0.2f));
    g.drawRoundedRectangle(bounds, cornerSize, 1.0f);
}

// Modern ComboBox with clean lines
void ModernLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
    int buttonX, int buttonY, int buttonW, int buttonH,
    juce::ComboBox& box) {
    auto bounds = juce::Rectangle<float>(0, 0, width, height);
    auto cornerSize = modernCornerRadius * 0.6f;

    // Background
    g.setColour(box.findColour(juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle(bounds, cornerSize);

    // Border
    g.setColour(box.findColour(juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle(bounds.reduced(0.5f), cornerSize, 1.0f);

    // Arrow area background
    auto arrowZone = juce::Rectangle<float>(buttonX, buttonY, buttonW, buttonH);
    g.setColour(box.findColour(juce::ComboBox::backgroundColourId).darker(0.05f));
    g.fillRoundedRectangle(arrowZone, cornerSize);

    // Modern arrow icon
    auto arrowX = buttonX + buttonW * 0.3f;
    auto arrowY = buttonY + buttonH * 0.35f;
    auto arrowW = buttonW * 0.4f;
    auto arrowH = buttonH * 0.3f;

    juce::Path arrow;
    arrow.startNewSubPath(arrowX, arrowY);
    arrow.lineTo(arrowX + arrowW * 0.5f, arrowY + arrowH);
    arrow.lineTo(arrowX + arrowW, arrowY);

    g.setColour(box.findColour(juce::ComboBox::arrowColourId));
    g.strokePath(arrow, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

// Modern popup menu with blur effect simulation
void ModernLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int width, int height) {
    auto bounds = juce::Rectangle<float>(0, 0, width, height);

    // Background with slight transparency
    g.setColour(findColour(juce::PopupMenu::backgroundColourId));
    g.fillRoundedRectangle(bounds, modernCornerRadius);

    // Subtle border
    g.setColour(findColour(juce::PopupMenu::backgroundColourId).contrasting(0.1f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), modernCornerRadius, 1.0f);
}

// Modern text editor with focus states
void ModernLookAndFeel::drawTextEditorOutline(juce::Graphics& g, int width, int height,
    juce::TextEditor& textEditor) {
    auto bounds = juce::Rectangle<float>(0, 0, width, height);
    auto cornerSize = modernCornerRadius * 0.5f;

    if (textEditor.hasKeyboardFocus(true)) {
        // Focused state with accent color
        g.setColour(textEditor.findColour(juce::TextEditor::focusedOutlineColourId));
        g.drawRoundedRectangle(bounds.reduced(1.0f), cornerSize, 2.0f);

        // Glow effect
        g.setColour(textEditor.findColour(juce::TextEditor::focusedOutlineColourId).withAlpha(0.3f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), cornerSize, 3.0f);
    }
    else {
        // Normal state
        g.setColour(textEditor.findColour(juce::TextEditor::outlineColourId));
        g.drawRoundedRectangle(bounds.reduced(0.5f), cornerSize, 1.0f);
    }
}

// Minimal modern scrollbar
void ModernLookAndFeel::drawScrollbar(juce::Graphics& g, juce::ScrollBar& scrollbar,
    int x, int y, int width, int height,
    bool isScrollbarVertical, int thumbStartPosition,
    int thumbSize, bool isMouseOver, bool isMouseDown) {
    // Track (background)
    g.setColour(scrollbar.findColour(juce::ScrollBar::backgroundColourId));
    g.fillRect(x, y, width, height);

    // Thumb
    juce::Rectangle<int> thumbBounds;

    if (isScrollbarVertical) {
        thumbBounds = juce::Rectangle<int>(x + 2, thumbStartPosition, width - 4, thumbSize);
    }
    else {
        thumbBounds = juce::Rectangle<int>(thumbStartPosition, y + 2, thumbSize, height - 4);
    }

    auto thumbColour = scrollbar.findColour(juce::ScrollBar::thumbColourId);

    if (isMouseDown) {
        thumbColour = thumbColour.brighter(0.1f);
    }
    else if (isMouseOver) {
        thumbColour = thumbColour.brighter(0.05f);
    }

    g.setColour(thumbColour);
    g.fillRoundedRectangle(thumbBounds.toFloat(), 3.0f);
}

// Modern typography
juce::Font ModernLookAndFeel::getTextButtonFont(juce::TextButton&, int buttonHeight) {
    return juce::Font("Inter", "Regular", juce::jmin(16.0f, buttonHeight * 0.6f));
}

juce::Font ModernLookAndFeel::getComboBoxFont(juce::ComboBox&) {
    return juce::Font("Inter", "Regular", 14.0f);
}

juce::Font ModernLookAndFeel::getLabelFont(juce::Label&) {
    return juce::Font("Inter", "Regular", 14.0f);
}

// ORIGINAL SLIDER METHODS - COMPLETELY UNTOUCHED
void ModernLookAndFeel::drawLinearSlider(Graphics& g, int x, int y, int width, int height,
    float sliderPos, float minSliderPos, float maxSliderPos,
    const Slider::SliderStyle style, Slider& slider) {
    g.fillAll(slider.findColour(Slider::backgroundColourId));

    if (style == Slider::SliderStyle::LinearBarVertical) {

        float from = (sliderPos / maxSliderPos) * height;
        float to = height;

        g.setGradientFill(gradientVertical);

        if (Time::getMillisecondCounter() - triggerTime > 2000) {
            triggerTime = Time::getMillisecondCounter();
            maxY = 0xFFFF;
        }

        if (from < maxY) {
            maxY = from;

        }

        g.fillRect((float)x, (float)maxY, (float)width, 1.0f);
        g.fillRect((float)x, from, (float)width, to);

    }
    else if (style == Slider::SliderStyle::LinearBar) {

        // float to  = (sliderPos / maxSliderPos) * width;
        //float to = width;

        g.setGradientFill(gradientHorizontal);
        //g.fillRect((float)x,(float)y,(float)to,(float)height);

        if (Time::getMillisecondCounter() - triggerTime > 2000) {
            triggerTime = Time::getMillisecondCounter();
            maxX = 0;
        }

        if (sliderPos > maxX) {
            maxX = sliderPos;

        }

        g.fillRect((float)maxX, (float)y, 2.0f, (float)height);
        g.fillRect(0.0f, (float)y, (float)(sliderPos), (float)height);
    }


    else if (style == Slider::SliderStyle::LinearVertical) {
        Image fader = ImageCache::getFromMemory(BinaryData::Fader_png, BinaryData::Fader_pngSize);


        float from = (sliderPos / maxSliderPos) * height;
        LookAndFeel_V3::drawLinearSliderBackground(g, x, y + 10, width, height + 10, sliderPos, minSliderPos, maxSliderPos, style, slider);


        g.setColour(Colours::white);
        g.drawImageAt(fader.rescaled(fader.getWidth(), fader.getHeight() / 2), 0, from - fader.getHeight() / 4 + 10);

    }
    else {
        LookAndFeel_V3::drawLinearSlider(g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
    }


}

void ModernLookAndFeel::drawRotarySlider(Graphics& g,
    int     x,
    int     y,
    int     width,
    int     height,
    float     sliderPosProportional,
    float     rotaryStartAngle,
    float     rotaryEndAngle,
    Slider& slider)
{
    //LookAndFeel_V2::drawRotarySlider(g, x, y, width, height, sliderPosProportional, rotaryStartAngle, rotaryEndAngle, slider);

    // This is the binary image data that uses very little CPU when rotating
    Image myStrip = ImageCache::getFromMemory(BinaryData::Knob_64_png, BinaryData::Knob_64_pngSize);

    const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
    const float radius = jmin(width / 2.0f, height / 2.0f);
    const float centreX = x + width * 0.5f;
    const float centreY = y + height * 0.5f;
    const float rx = centreX - radius - 1.0f;
    const float ry = centreY - radius - 1.0f;
    const float rw = radius * 2.0f;
    const float thickness = 0.9f;

    // const double fractRotation = (slider.getValue() - slider.getMinimum())  /
    //(slider.getMaximum() - slider.getMinimum()); //value between 0 and 1 for current amount of rotation

    const int nFrames = myStrip.getHeight() / myStrip.getWidth(); // number of frames for vertical film strip
    const int frameIdx = (int)ceil(sliderPosProportional * ((double)nFrames - 1.0)); // current index from 0 --> nFrames-1

    // Logger::getCurrentLogger()->writeToLog("==========" +String(sliderPosProportional));

    g.setColour(Colours::darkorange);
    {
        Path filledArc;

        float dist = angle - rotaryStartAngle;

        for (float a = rotaryStartAngle; a < dist + rotaryStartAngle; a += 0.15) {
            filledArc.addPieSegment(rx + 1, ry + 1, rw - 0.5, rw - 0.5, a, a + 0.1, thickness);
        }

        // filledArc.addPieSegment (rx+1, ry+1, rw-0.5, rw-0.5, rotaryStartAngle, angle, thickness);

        g.fillPath(filledArc);
    }

    g.drawImage(myStrip,
        (int)rx,
        (int)ry,
        2 * (int)radius,
        2 * (int)radius,   //Dest
        0,
        frameIdx * myStrip.getWidth(),
        myStrip.getWidth(),
        myStrip.getWidth()); //Source

}