/*
  ==============================================================================

    ModernLookAndFeel.h
    Created: 6 Sep 2025 9:13:02pm
    Author:  mpue

  ==============================================================================
*/

/*
  ==============================================================================

    ModernLookAndFeel.h
    Created: Modern adaptation of CustomLookAndFeel
    Author:  Based on Matthias Pueski's work

  ==============================================================================
*/

#ifndef MODERNLOOKANDFEEL_H_INCLUDED
#define MODERNLOOKANDFEEL_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"

class ModernLookAndFeel : public juce::LookAndFeel_V4 {

public:
    ModernLookAndFeel() {
        // Modern color scheme - dark theme with accent colors
        auto modernScheme = LookAndFeel_V4::getDarkColourScheme();
        setColourScheme(modernScheme);

        // Modern gradient definitions
        gradientVertical = juce::ColourGradient(
            juce::Colour(0xff6366f1), 0, 0,     // Indigo
            juce::Colour(0xff06b6d4), 5, 128,   // Cyan
            false
        );
        gradientVertical.addColour(0.3, juce::Colour(0xff8b5cf6)); // Purple
        gradientVertical.addColour(0.7, juce::Colour(0xff10b981)); // Emerald

        gradientHorizontal = juce::ColourGradient(
            juce::Colour(0xff06b6d4), 0, 0,     // Cyan
            juce::Colour(0xff6366f1), 128, 5,   // Indigo
            false
        );
        gradientHorizontal.addColour(0.5, juce::Colour(0xff8b5cf6)); // Purple

        // Modern color palette
        const auto primaryBg = juce::Colour(0xff1e1e2e);      // Dark background
        const auto secondaryBg = juce::Colour(0xff2a2a3e);    // Slightly lighter
        const auto surfaceBg = juce::Colour(0xff363651);      // Surface color
        const auto accentPrimary = juce::Colour(0xff10b981);  // Indigo accent
        const auto accentSecondary = juce::Colour(0xff06b6d4); // Cyan accent
        const auto textPrimary = juce::Colour(0xffe2e8f0);    // Light text
        const auto textSecondary = juce::Colour(0xffa0a9c0);  // Muted text
        const auto borderColor = juce::Colour(0xff4a4a6a);    // Subtle borders

        // Window and dialog colors
        setColour(juce::DialogWindow::backgroundColourId, primaryBg);
        setColour(juce::ResizableWindow::backgroundColourId, primaryBg);
        setColour(juce::AlertWindow::backgroundColourId, secondaryBg);
        setColour(juce::AlertWindow::outlineColourId, borderColor);
        setColour(juce::DocumentWindow::textColourId, textPrimary);

        // Text and labels
        setColour(juce::Label::textColourId, textPrimary);
        setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

        // Buttons - modern flat design with subtle gradients
        setColour(juce::TextButton::buttonColourId, surfaceBg);
        setColour(juce::TextButton::buttonOnColourId, accentPrimary);
        setColour(juce::TextButton::textColourOnId, juce::Colours::white);
        setColour(juce::TextButton::textColourOffId, textPrimary);

        // ComboBox - modern dropdown styling
        setColour(juce::ComboBox::backgroundColourId, surfaceBg);
        setColour(juce::ComboBox::textColourId, textPrimary);
        setColour(juce::ComboBox::outlineColourId, borderColor);
        setColour(juce::ComboBox::arrowColourId, accentSecondary);
        setColour(juce::ComboBox::buttonColourId, juce::Colours::transparentBlack);

        // PopupMenu - modern context menus
        setColour(juce::PopupMenu::backgroundColourId, secondaryBg);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, accentPrimary.withAlpha(0.3f));
        setColour(juce::PopupMenu::textColourId, textPrimary);
        setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white);

        // ListBox - modern list styling
        setColour(juce::ListBox::backgroundColourId, primaryBg);
        setColour(juce::ListBox::textColourId, textPrimary);
        setColour(juce::ListBox::outlineColourId, borderColor);

        // TextEditor - modern input fields
        setColour(juce::TextEditor::backgroundColourId, surfaceBg);
        setColour(juce::TextEditor::textColourId, textPrimary);
        setColour(juce::TextEditor::outlineColourId, borderColor);
        setColour(juce::TextEditor::focusedOutlineColourId, accentPrimary);
        setColour(juce::TextEditor::highlightColourId, accentPrimary.withAlpha(0.3f));
        setColour(juce::TextEditor::highlightedTextColourId, juce::Colours::white);

        // Scrollbars - minimal modern design
        setColour(juce::ScrollBar::backgroundColourId, primaryBg);
        setColour(juce::ScrollBar::thumbColourId, borderColor);
        setColour(juce::ScrollBar::trackColourId, juce::Colours::transparentBlack);

        // Sliders - keeping original behavior but updating colors
        setColour(juce::Slider::rotarySliderFillColourId, accentPrimary);
        setColour(juce::Slider::trackColourId, surfaceBg);
        setColour(juce::Slider::thumbColourId, accentSecondary);
        setColour(juce::Slider::textBoxTextColourId, textPrimary);
        setColour(juce::Slider::textBoxBackgroundColourId, surfaceBg);
        setColour(juce::Slider::textBoxOutlineColourId, borderColor);

        // Caret
        setColour(juce::CaretComponent::caretColourId, accentPrimary);

        // Tooltips
        setColour(juce::TooltipWindow::backgroundColourId, secondaryBg);
        setColour(juce::TooltipWindow::textColourId, textPrimary);
        setColour(juce::TooltipWindow::outlineColourId, borderColor);
    }

    // Override methods for modern styling
    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
        const juce::Colour& backgroundColour,
        bool shouldDrawButtonAsHighlighted,
        bool shouldDrawButtonAsDown) override;

    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
        int buttonX, int buttonY, int buttonW, int buttonH,
        juce::ComboBox& box) override;

    void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override;

    void drawTextEditorOutline(juce::Graphics& g, int width, int height,
        juce::TextEditor& textEditor) override;

    void drawScrollbar(juce::Graphics& g, juce::ScrollBar& scrollbar,
        int x, int y, int width, int height,
        bool isScrollbarVertical, int thumbStartPosition,
        int thumbSize, bool isMouseOver, bool isMouseDown) override;

    juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override;
    juce::Font getComboBoxFont(juce::ComboBox&) override;
    juce::Font getLabelFont(juce::Label&) override;

    // Keep original slider methods untouched
    void drawLinearSlider(juce::Graphics&, int x, int y, int width, int height,
        float sliderPos, float minSliderPos, float maxSliderPos,
        const juce::Slider::SliderStyle, juce::Slider&) override;

    void drawRotarySlider(juce::Graphics& g,
        int     x,
        int     y,
        int     width,
        int     height,
        float     sliderPosProportional,
        float     rotaryStartAngle,
        float     rotaryEndAngle,
        juce::Slider& slider) override;

private:
    juce::ColourGradient gradientVertical;
    juce::ColourGradient gradientHorizontal;
    juce::ComponentAnimator animator;

    long triggerTime = 0;
    int maxY = 0xFFFF;
    int maxX = 0;

    // Modern design constants
    static constexpr float modernCornerRadius = 8.0f;
    static constexpr float shadowOffset = 2.0f;
    static constexpr float shadowRadius = 8.0f;
};

#endif  // MODERNLOOKANDFEEL_H_INCLUDED
