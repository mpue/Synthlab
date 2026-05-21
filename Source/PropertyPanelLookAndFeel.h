/*
  ==============================================================================

    PropertyPanelLookAndFeel.h
    Author:  Matthias Pueski

    LookAndFeel for the PropertyView's PropertyPanel only. Subclasses
    LookAndFeel_V4 directly (NOT ElegantDarkLookAndFeel) so we drop the
    chartreuse/red LinearBar gradient that ElegantDarkLookAndFeel bakes in
    for the synth-module knobs — that gradient looks wrong in slim property
    strips. Everything else (knobs, faders elsewhere) keeps the elegant-dark
    paint by virtue of the parent-chain LookAndFeel staying untouched; only
    the propertyPanel adopts this one via setLookAndFeel().

    Header-only so adding it to the project doesn't require a Projucer
    regeneration step.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

class PropertyPanelLookAndFeel : public juce::LookAndFeel_V4
{
public:
    PropertyPanelLookAndFeel()
    {
        // Palette mirrors PropertyView's local constants — kept here so the
        // L&F can be reused without re-passing the colours.
        const auto stripBg   = juce::Colour (0xff222222);  // matches PropertyView bg
        const auto trackBg   = juce::Colour (0xff2a2d33);  // empty part of the bar
        const auto fillCol   = juce::Colour (0xff5a8fff);  // accent — value bar
        const auto textHi    = juce::Colour (0xffe8e9ec);
        const auto textLo    = juce::Colour (0xffa8acb4);
        const auto separator = juce::Colour (0xff2a2d33);

        // Slider — used by SliderPropertyComponent. textBox is overlaid for
        // LinearBar, drawn transparent so our flat track shows through.
        setColour (juce::Slider::backgroundColourId,        trackBg);
        setColour (juce::Slider::trackColourId,             fillCol);
        setColour (juce::Slider::thumbColourId,             fillCol);
        setColour (juce::Slider::textBoxTextColourId,       textHi);
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxOutlineColourId,    juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxHighlightColourId,  fillCol.withAlpha (0.4f));

        // PropertyComponent row chrome.
        setColour (juce::PropertyComponent::backgroundColourId, stripBg);
        setColour (juce::PropertyComponent::labelTextColourId,  textHi);

        // BooleanPropertyComponent (the bordered toggle box).
        setColour (juce::BooleanPropertyComponent::backgroundColourId, stripBg);
        setColour (juce::BooleanPropertyComponent::outlineColourId,    separator);

        // TextPropertyComponent (a TextEditor in a frame).
        setColour (juce::TextPropertyComponent::backgroundColourId, stripBg);
        setColour (juce::TextPropertyComponent::textColourId,       textHi);
        setColour (juce::TextPropertyComponent::outlineColourId,    separator);

        // TextEditor used inside TextPropertyComponent etc.
        setColour (juce::TextEditor::backgroundColourId,     stripBg);
        setColour (juce::TextEditor::textColourId,           textHi);
        setColour (juce::TextEditor::outlineColourId,        separator);
        setColour (juce::TextEditor::focusedOutlineColourId, fillCol.withAlpha (0.6f));

        // ToggleButton (BooleanPropertyComponent).
        setColour (juce::ToggleButton::textColourId, textHi);
        setColour (juce::ToggleButton::tickColourId, fillCol);

        // ComboBox — used by ChoicePropertyComponent (dropdown). The default
        // V4 colours are a pale grey that pops against the dark strips; pull
        // everything onto stripBg so the box blends and only the value text
        // / arrow are visible.
        setColour (juce::ComboBox::backgroundColourId,      stripBg);
        setColour (juce::ComboBox::textColourId,            textHi);
        setColour (juce::ComboBox::outlineColourId,         separator);
        setColour (juce::ComboBox::buttonColourId,          stripBg);   // arrow button bg
        setColour (juce::ComboBox::arrowColourId,           textLo);
        setColour (juce::ComboBox::focusedOutlineColourId,  fillCol.withAlpha (0.6f));

        // PopupMenu = the list that opens when the ComboBox is clicked.
        // Without these, JUCE falls back to the platform default (white on
        // macOS) which is jarring against the dark editor.
        setColour (juce::PopupMenu::backgroundColourId,             stripBg);
        setColour (juce::PopupMenu::textColourId,                   textHi);
        setColour (juce::PopupMenu::headerTextColourId,             textLo);
        setColour (juce::PopupMenu::highlightedBackgroundColourId,  fillCol.withAlpha (0.35f));
        setColour (juce::PopupMenu::highlightedTextColourId,        textHi);

        // Section-header text (used in drawPropertyPanelSectionHeader below).
        setColour (juce::TextButton::textColourOffId, textLo);

        // Make sure the resizable-window bg id (consumed by the section
        // header's +/- box) blends instead of showing the platform default.
        setColour (juce::ResizableWindow::backgroundColourId, stripBg);
    }

    // V4's drawLinearSlider does NOT fillAll for non-bar styles, and for bar
    // styles only paints the value rectangle in trackColourId — relying on
    // the parent to have provided a background. That works in plain JUCE,
    // but the parent-chain bg here can leak through transparent regions of
    // the propertyHolder, producing the "default JUCE colour" effect the
    // user complained about. We explicitly fill the slider's bounds first.
    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           const juce::Slider::SliderStyle style,
                           juce::Slider& slider) override
    {
        if (style == juce::Slider::LinearBar || style == juce::Slider::LinearBarVertical)
        {
            // Flat backdrop — the empty part of the bar.
            g.fillAll (slider.findColour (juce::Slider::backgroundColourId));

            // Filled portion — flat, no gradient, slight transparency so the
            // value text overlay still reads well at full deflection.
            g.setColour (slider.findColour (juce::Slider::trackColourId).withAlpha (0.55f));

            if (style == juce::Slider::LinearBar)
            {
                const float fillW = juce::jmax (0.0f, sliderPos - (float) x);
                g.fillRect (juce::Rectangle<float> ((float) x, (float) y, fillW, (float) height));
            }
            else  // LinearBarVertical
            {
                const float fillTop = sliderPos;
                const float fillH   = juce::jmax (0.0f, (float) (y + height) - sliderPos);
                g.fillRect (juce::Rectangle<float> ((float) x, fillTop, (float) width, fillH));
            }
        }
        else
        {
            juce::LookAndFeel_V4::drawLinearSlider (g, x, y, width, height,
                                                    sliderPos, minSliderPos, maxSliderPos,
                                                    style, slider);
        }
    }

    // V4 default: textW = min(200, width/2) — half the strip for a label,
    // no right margin so the slider's right edge touches the panel edge
    // (or worse, a scrollbar). Slim panel + long labels = unusable slider.
    //
    // Ours: cap label width tighter (160 px max, ~40 % of strip) AND leave
    // an 8 px right gutter so the slider visibly breathes from the panel
    // edge / scrollbar.
    juce::Rectangle<int> getPropertyComponentContentPosition (juce::PropertyComponent& component) override
    {
        const int rightMargin = 8;
        const int textW       = juce::jmin (160, component.getWidth() * 4 / 10);
        const int contentW    = juce::jmax (40, component.getWidth() - textW - rightMargin);
        return { textW, 0, contentW, component.getHeight() - 1 };
    }

    // (Note: getPropertyComponentIndent in LookAndFeel_V4 is not virtual —
    // it's an internal helper of drawPropertyComponentLabel. Overriding it
    // would need a full drawPropertyComponentLabel override too. The V4
    // default of min(10, width/10) is fine; not worth re-rolling the label
    // paint just to shave a few pixels.)

    // V4 doesn't fill the section-header background — only paints the +/-
    // box and the title text. The strip between sections then shows the
    // parent's bg (often the JUCE-default light grey if no one has filled
    // it). We fill first with PropertyComponent::backgroundColourId so the
    // header blends into the stripBg of the data rows above and below.
    void drawPropertyPanelSectionHeader (juce::Graphics& g, const juce::String& name,
                                         bool isOpen, int width, int height) override
    {
        g.fillAll (findColour (juce::PropertyComponent::backgroundColourId));
        juce::LookAndFeel_V4::drawPropertyPanelSectionHeader (g, name, isOpen, width, height);
    }
};
