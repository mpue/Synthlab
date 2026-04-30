/*
  ==============================================================================

    ElegantDarkLookAndFeel.h
    Created: 27 Sep 2025 9:53:04pm
    Author:  mpue

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class ElegantDarkLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ElegantDarkLookAndFeel()
    {
        // Basis Farbschema
        setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xff1a1a1a));
        setColour(juce::DocumentWindow::backgroundColourId, juce::Colour(0xff1a1a1a));

        // Text Farben
        setColour(juce::Label::textColourId, juce::Colour(0xffe8e8e8));
        setColour(juce::TextEditor::textColourId, juce::Colour(0xffe8e8e8));
        setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff2a2a2a));
        setColour(juce::TextEditor::highlightColourId, juce::Colour(0x664d9eff));
        setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff404040));
        setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(0xff4d9eff));

        // Button Farben
        setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2a2a2a));
        setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff4d9eff));
        setColour(juce::TextButton::textColourOffId, juce::Colour(0xffe8e8e8));
        setColour(juce::TextButton::textColourOnId, juce::Colour(0xffffffff));

        // ComboBox
        setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff2a2a2a));
        setColour(juce::ComboBox::textColourId, juce::Colour(0xffe8e8e8));
        setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff404040));
        setColour(juce::ComboBox::buttonColourId, juce::Colour(0xff404040));
        setColour(juce::ComboBox::arrowColourId, juce::Colour(0xffe8e8e8));

        // PopupMenu
        setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff2a2a2a));
        setColour(juce::PopupMenu::textColourId, juce::Colour(0xffe8e8e8));
        setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff4d9eff));
        setColour(juce::PopupMenu::highlightedTextColourId, juce::Colour(0xffffffff));

        // Slider
        setColour(juce::Slider::backgroundColourId, juce::Colour(0xff404040));
        setColour(juce::Slider::thumbColourId, juce::Colour(0xff4d9eff));
        setColour(juce::Slider::trackColourId, juce::Colour(0xff666666));
        setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff4d9eff));
        setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff404040));
        setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffe8e8e8));
        setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff2a2a2a));
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff404040));

        // ToggleButton
        setColour(juce::ToggleButton::textColourId, juce::Colour(0xffe8e8e8));
        setColour(juce::ToggleButton::tickColourId, juce::Colour(0xff4d9eff));
        setColour(juce::ToggleButton::tickDisabledColourId, juce::Colour(0xff666666));

        // ListBox
        setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff2a2a2a));
        setColour(juce::ListBox::outlineColourId, juce::Colour(0xff404040));
        setColour(juce::ListBox::textColourId, juce::Colour(0xffe8e8e8));

        // TreeView
        setColour(juce::TreeView::backgroundColourId, juce::Colour(0xff2a2a2a));
        setColour(juce::TreeView::linesColourId, juce::Colour(0xff404040));
        setColour(juce::TreeView::dragAndDropIndicatorColourId, juce::Colour(0xff4d9eff));
        setColour(juce::TreeView::selectedItemBackgroundColourId, juce::Colour(0x664d9eff));

        // Scrollbar
        setColour(juce::ScrollBar::backgroundColourId, juce::Colour(0xff1a1a1a));
        setColour(juce::ScrollBar::thumbColourId, juce::Colour(0xff4d9eff));
        setColour(juce::ScrollBar::trackColourId, juce::Colour(0xff2a2a2a));

        // TabbedComponent
        setColour(juce::TabbedComponent::backgroundColourId, juce::Colour(0xff1a1a1a));
        setColour(juce::TabbedComponent::outlineColourId, juce::Colour(0xff404040));
        setColour(juce::TabbedButtonBar::tabOutlineColourId, juce::Colour(0xff404040));
        setColour(juce::TabbedButtonBar::frontOutlineColourId, juce::Colour(0xff4d9eff));

        // TableListBox
        setColour(juce::TableListBox::backgroundColourId, juce::Colour(0xff2a2a2a));
        setColour(juce::TableListBox::outlineColourId, juce::Colour(0xff404040));
        setColour(juce::TableHeaderComponent::backgroundColourId, juce::Colour(0xff333333));
        setColour(juce::TableHeaderComponent::outlineColourId, juce::Colour(0xff404040));
        setColour(juce::TableHeaderComponent::textColourId, juce::Colour(0xffe8e8e8));
        setColour(juce::TableHeaderComponent::highlightColourId, juce::Colour(0xff4d9eff));

        // ProgressBar
        setColour(juce::ProgressBar::backgroundColourId, juce::Colour(0xff404040));
        setColour(juce::ProgressBar::foregroundColourId, juce::Colour(0xff4d9eff));

        // GroupComponent
        setColour(juce::GroupComponent::outlineColourId, juce::Colour(0xff404040));
        setColour(juce::GroupComponent::textColourId, juce::Colour(0xffe8e8e8));

        // DirectoryContentsDisplayComponent
        setColour(juce::DirectoryContentsDisplayComponent::highlightColourId, juce::Colour(0x664d9eff));
        setColour(juce::DirectoryContentsDisplayComponent::textColourId, juce::Colour(0xffe8e8e8));

        // FileBrowserComponent
        setColour(juce::FileBrowserComponent::currentPathBoxBackgroundColourId, juce::Colour(0xff2a2a2a));
        setColour(juce::FileBrowserComponent::currentPathBoxTextColourId, juce::Colour(0xffe8e8e8));
        setColour(juce::FileBrowserComponent::currentPathBoxArrowColourId, juce::Colour(0xffe8e8e8));
        setColour(juce::FileBrowserComponent::filenameBoxBackgroundColourId, juce::Colour(0xff2a2a2a));
        setColour(juce::FileBrowserComponent::filenameBoxTextColourId, juce::Colour(0xffe8e8e8));

        // AlertWindow
        setColour(juce::AlertWindow::backgroundColourId, juce::Colour(0xff2a2a2a));
        setColour(juce::AlertWindow::textColourId, juce::Colour(0xffe8e8e8));
        setColour(juce::AlertWindow::outlineColourId, juce::Colour(0xff404040));

        // TooltipWindow
        setColour(juce::TooltipWindow::backgroundColourId, juce::Colour(0xff333333));
        setColour(juce::TooltipWindow::textColourId, juce::Colour(0xffe8e8e8));
        setColour(juce::TooltipWindow::outlineColourId, juce::Colour(0xff404040));

        // CodeEditorComponent
        setColour(juce::CodeEditorComponent::backgroundColourId, juce::Colour(0xff1a1a1a));
        setColour(juce::CodeEditorComponent::highlightColourId, juce::Colour(0x664d9eff));
        setColour(juce::CodeEditorComponent::defaultTextColourId, juce::Colour(0xffe8e8e8));
        setColour(juce::CodeEditorComponent::lineNumberBackgroundId, juce::Colour(0xff2a2a2a));
        setColour(juce::CodeEditorComponent::lineNumberTextId, juce::Colour(0xff999999));

        // CaretComponent
        setColour(juce::CaretComponent::caretColourId, juce::Colour(0xff4d9eff));

        // HyperlinkButton
        setColour(juce::HyperlinkButton::textColourId, juce::Colour(0xff4d9eff));

        // KeyboardComponentBase
        setColour(juce::KeyboardComponentBase::upDownButtonBackgroundColourId, juce::Colour(0xff2a2a2a));
        setColour(juce::KeyboardComponentBase::upDownButtonArrowColourId, juce::Colour(0xffe8e8e8));

        // MidiKeyboardComponent
        setColour(juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour(0xffe8e8e8));
        setColour(juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour(0xff1a1a1a));
        setColour(juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour(0xff404040));
        setColour(juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, juce::Colour(0x664d9eff));
        setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId, juce::Colour(0x994d9eff));
        setColour(juce::MidiKeyboardComponent::textLabelColourId, juce::Colour(0xff1a1a1a));
        setColour(juce::MidiKeyboardComponent::upDownButtonBackgroundColourId, juce::Colour(0xff2a2a2a));
        setColour(juce::MidiKeyboardComponent::upDownButtonArrowColourId, juce::Colour(0xffe8e8e8));

		setColour(juce::TabbedComponent::backgroundColourId, juce::Colour(0xff1a1a1a));

        gradientVertical = juce::ColourGradient(juce::Colours::red, 0, 0, juce::Colours::chartreuse, 5, 128, false);
        gradientVertical.addColour(0.2, juce::Colours::lightgrey);

        gradientHorizontal = juce::ColourGradient(juce::Colours::chartreuse, 0, 0, juce::Colours::red, 128, 5, false);
        gradientHorizontal.addColour(0.8, juce::Colours::grey);

        // Schriftart setzen
        setDefaultSansSerifTypefaceName("Verdana");
    }

    // Custom Button Drawing mit abgerundeten Ecken und Glow-Effekt
    void drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
        bool isMouseOverButton, bool isButtonDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(0.5f, 0.5f);
        auto cornerSize = 8.0f;

        if (isButtonDown)
        {
            g.setColour(backgroundColour.brighter(0.2f));
        }
        else if (isMouseOverButton)
        {
            g.setColour(backgroundColour.brighter(0.1f));

            // Glow-Effekt f�r Hover
            juce::Path glowPath;
            glowPath.addRoundedRectangle(bounds.expanded(2.0f), cornerSize + 2.0f);
            juce::ColourGradient glow(juce::Colour(0x334d9eff), bounds.getCentreX(), bounds.getCentreY(),
                juce::Colour(0x004d9eff), bounds.getRight(), bounds.getBottom(), true);
            g.setGradientFill(glow);
            g.fillPath(glowPath);
        }
        else
        {
            g.setColour(backgroundColour);
        }

        g.fillRoundedRectangle(bounds, cornerSize);

        // Subtiler Outline
        g.setColour(juce::Colour(0xff404040));
        g.drawRoundedRectangle(bounds, cornerSize, 1.0f);
    }

    // Hardware-style rotary knob.
    // Layers (back to front): tick marks → background track arc → soft glow
    // halo on the lit portion → sharp accent value arc → drop shadow → bevel
    // ring → metallic body gradient → glassy upper-left highlight → rim
    // outline → recessed indent line → glowing LED indicator at the tip.
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
        const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider& slider) override
    {
        const float diameter = (float) juce::jmin(width, height);
        const float centreX  = (float) x + (float) width  * 0.5f;
        const float centreY  = (float) y + (float) height * 0.5f;
        const float outerR   = diameter * 0.5f - 2.0f;
        const float trackR   = outerR - 1.0f;

        const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        const juce::Colour accent      = slider.findColour(juce::Slider::rotarySliderFillColourId);
        const juce::Colour accentBright = accent.brighter(0.40f);
        const juce::Colour accentDim    = accent.withAlpha(0.22f);

        const juce::Colour bevelLight (0xff70747a);
        const juce::Colour bevelDark  (0xff0e0e10);
        const juce::Colour bodyTop    (0xff5a5d63);
        const juce::Colour bodyMid    (0xff35373b);
        const juce::Colour bodyBottom (0xff1d1e21);
        const juce::Colour trackBg    (0xff141416);

        const bool active = slider.isEnabled();
        const bool hot    = active && (slider.isMouseOverOrDragging() || slider.hasKeyboardFocus(true));

        const float halfPi = juce::MathConstants<float>::halfPi;
        auto polar = [&](float a, float r, float& outX, float& outY) {
            outX = centreX + std::cos(a - halfPi) * r;
            outY = centreY + std::sin(a - halfPi) * r;
        };

        // --- 1) Tick marks around the perimeter -----------------------------
        if (diameter >= 24.0f) {
            const int   numTicks  = 11;
            const float tickInner = outerR + 0.5f;
            const float tickOuter = outerR + 3.0f;
            g.setColour(juce::Colours::white.withAlpha(active ? 0.18f : 0.10f));
            for (int i = 0; i < numTicks; ++i) {
                const float t = (float) i / (float) (numTicks - 1);
                const float a = rotaryStartAngle + t * (rotaryEndAngle - rotaryStartAngle);
                float ix, iy, ox, oy;
                polar(a, tickInner, ix, iy);
                polar(a, tickOuter, ox, oy);
                g.drawLine(ix, iy, ox, oy, 1.0f);
            }
        }

        // --- 2) Background track arc (full sweep) ---------------------------
        {
            juce::Path bg;
            bg.addCentredArc(centreX, centreY, trackR, trackR, 0.0f,
                             rotaryStartAngle, rotaryEndAngle, true);
            g.setColour(trackBg);
            g.strokePath(bg, juce::PathStrokeType(3.5f, juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));
        }

        // --- 3) Soft halo behind the value arc ------------------------------
        if (active) {
            juce::Path halo;
            halo.addCentredArc(centreX, centreY, trackR, trackR, 0.0f,
                               rotaryStartAngle, angle, true);
            g.setColour(hot ? accent.withAlpha(0.40f) : accentDim);
            g.strokePath(halo, juce::PathStrokeType(8.5f, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
        }

        // --- 4) Foreground value arc (sharp accent line) --------------------
        {
            juce::Path val;
            val.addCentredArc(centreX, centreY, trackR, trackR, 0.0f,
                              rotaryStartAngle, angle, true);
            g.setColour(active ? (hot ? accentBright : accent) : juce::Colour(0xff5a5a5a));
            g.strokePath(val, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
        }

        // --- 5) Knob body — drop shadow + bevel ring + metallic interior ----
        const float bodyR = outerR - 6.0f;
        if (bodyR <= 1.0f)
            return; // too small to draw a meaningful body
        const float bx = centreX - bodyR;
        const float by = centreY - bodyR;
        const float bw = bodyR * 2.0f;

        // Drop shadow
        g.setColour(juce::Colours::black.withAlpha(0.45f));
        g.fillEllipse(bx + 1.0f, by + 2.0f, bw, bw);

        // Outer bevel — diagonal light gradient
        {
            juce::ColourGradient bevel(bevelLight, bx,        by,
                                       bevelDark,  bx + bw,   by + bw, false);
            g.setGradientFill(bevel);
            g.fillEllipse(bx, by, bw, bw);
        }

        // Inner body — slightly inset, vertical brushed-metal gradient
        const float innerR = bodyR - 2.0f;
        const float ix = centreX - innerR;
        const float iy = centreY - innerR;
        const float iw = innerR * 2.0f;
        {
            juce::ColourGradient body(bodyTop,    ix, iy,
                                      bodyBottom, ix, iy + iw, false);
            body.addColour(0.5, bodyMid);
            g.setGradientFill(body);
            g.fillEllipse(ix, iy, iw, iw);
        }

        // Glassy radial highlight on upper-left
        {
            const float hr = innerR * 0.9f;
            juce::ColourGradient hi(juce::Colours::white.withAlpha(0.20f),
                                    centreX - innerR * 0.4f, centreY - innerR * 0.4f,
                                    juce::Colours::white.withAlpha(0.0f),
                                    centreX + hr, centreY + hr, true);
            g.setGradientFill(hi);
            g.fillEllipse(ix, iy, iw, iw);
        }

        // Rim outline + inner darker ring (chamfer)
        g.setColour(juce::Colours::black.withAlpha(0.7f));
        g.drawEllipse(ix, iy, iw, iw, 1.0f);
        g.setColour(juce::Colours::white.withAlpha(0.05f));
        g.drawEllipse(ix + 1.0f, iy + 1.0f, iw - 2.0f, iw - 2.0f, 1.0f);

        // --- 6) Indicator: recessed dark line with a glowing tip ------------
        {
            const float lineInner = innerR * 0.30f;
            const float lineOuter = innerR * 0.92f;

            float lx0, ly0, lx1, ly1;
            polar(angle, lineInner, lx0, ly0);
            polar(angle, lineOuter, lx1, ly1);

            // Dark recessed indent
            g.setColour(juce::Colours::black.withAlpha(0.85f));
            g.drawLine(lx0, ly0, lx1, ly1, 2.5f);
            // Faint highlight directly beside it (sells the recess)
            g.setColour(juce::Colours::white.withAlpha(0.10f));
            g.drawLine(lx0, ly0 + 1.0f, lx1, ly1 + 1.0f, 1.0f);

            // Glowing LED dot at the tip
            const float dotR = juce::jmax(2.0f, innerR * 0.11f);
            float dotX, dotY;
            polar(angle, lineOuter - dotR * 0.3f, dotX, dotY);

            if (active) {
                // halo
                juce::ColourGradient dotHalo(accentBright.withAlpha(hot ? 0.70f : 0.55f),
                                              dotX, dotY,
                                              accentBright.withAlpha(0.0f),
                                              dotX + dotR * 3.0f, dotY, true);
                g.setGradientFill(dotHalo);
                g.fillEllipse(dotX - dotR * 3.0f, dotY - dotR * 3.0f,
                              dotR * 6.0f,         dotR * 6.0f);
                g.setColour(accentBright);
            } else {
                g.setColour(juce::Colour(0xff7a7a7e));
            }
            g.fillEllipse(dotX - dotR, dotY - dotR, dotR * 2.0f, dotR * 2.0f);
            // Specular speck
            g.setColour(juce::Colours::white.withAlpha(0.85f));
            g.fillEllipse(dotX - dotR * 0.40f, dotY - dotR * 0.55f,
                          dotR * 0.6f,          dotR * 0.6f);
        }
    }

    /*
    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
        float sliderPos, float minSliderPos, float maxSliderPos,
        juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        if (slider.isBar())
        {
            g.setColour(slider.findColour(juce::Slider::trackColourId));
            g.fillRect(slider.isHorizontal() ? juce::Rectangle<float>(static_cast<float>(x), (float)y + (float)height * 0.4f,
                sliderPos - (float)x, (float)height * 0.2f)
                : juce::Rectangle<float>((float)x + (float)width * 0.4f, sliderPos,
                    (float)width * 0.2f, (float)y + (float)height - sliderPos));
        }
        else
        {
            auto trackWidth = juce::jmin(6.0f, slider.isHorizontal() ? (float)height * 0.25f : (float)width * 0.25f);

            juce::Point<float> startPoint(slider.isHorizontal() ? (float)x : (float)x + (float)width * 0.5f,
                slider.isHorizontal() ? (float)y + (float)height * 0.5f : (float)(height + y));

            juce::Point<float> endPoint(slider.isHorizontal() ? (float)(width + x) : startPoint.x,
                slider.isHorizontal() ? startPoint.y : (float)y);

            juce::Path backgroundTrack;
            backgroundTrack.startNewSubPath(startPoint);
            backgroundTrack.lineTo(endPoint);
            g.setColour(slider.findColour(juce::Slider::backgroundColourId));
            g.strokePath(backgroundTrack, { trackWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded });

            juce::Path valueTrack;
            juce::Point<float> minPoint, maxPoint, thumbPoint;

            if (slider.isHorizontal())
            {
                minPoint = startPoint;
                maxPoint = { sliderPos, startPoint.y };
                thumbPoint = { sliderPos, startPoint.y };
            }
            else
            {
                minPoint = { startPoint.x, sliderPos };
                maxPoint = endPoint;
                thumbPoint = { startPoint.x, sliderPos };
            }

            valueTrack.startNewSubPath(minPoint);
            valueTrack.lineTo(maxPoint);
            g.setColour(slider.findColour(juce::Slider::trackColourId));
            g.strokePath(valueTrack, { trackWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded });

            // Thumb mit Glow
            auto thumbRadius = trackWidth * 1.5f;
            g.setColour(juce::Colour(0x664d9eff));
            g.fillEllipse(juce::Rectangle<float>(thumbRadius * 2.0f, thumbRadius * 2.0f).withCentre(thumbPoint));
            g.setColour(slider.findColour(juce::Slider::thumbColourId));
            g.fillEllipse(juce::Rectangle<float>(thumbRadius, thumbRadius).withCentre(thumbPoint));
        }
    }

    */
   
    

    void drawLinearSlider(Graphics& g, int x, int y, int width, int height,
        float sliderPos, float minSliderPos, float maxSliderPos,
        const Slider::SliderStyle style, Slider& slider) {
        g.fillAll(slider.findColour(Slider::backgroundColourId));

        if (std::isnan(sliderPos)) {
            sliderPos = minSliderPos;
		}

        if (style == Slider::SliderStyle::LinearBarVertical) {

            float from = (sliderPos / maxSliderPos) * height;
            float to = height;

            g.setGradientFill(gradientVertical);

            if (Time::getMillisecondCounter() - triggerTime > 1000) {
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

            // Render the fader at half its native height (matches the original look).
            Image faderImg = fader.rescaled(fader.getWidth(), fader.getHeight() / 2);
            const int thumbW = faderImg.getWidth();
            const int thumbH = faderImg.getHeight();

            // Track background — drawn flush with the slider bounds; the previous
            // `+ 10` offsets were ad-hoc compensation for the broken thumb math
            // below and shifted the track noticeably out of place.
            LookAndFeel_V3::drawLinearSliderBackground(g, x, y, width, height,
                                                       sliderPos, minSliderPos, maxSliderPos,
                                                       style, slider);

            // sliderPos is *already* the pixel Y of the thumb's centre — the old
            // `(sliderPos / maxSliderPos) * height` re-derivation was wrong and
            // caused the fader to drift out of the slider bounds at the extremes,
            // so the bottom half of the handle got clipped at min position.
            // We now place the image directly at sliderPos and clamp the result
            // so it never overflows the slider bounds.
            const int thumbCentreY = (int) sliderPos;
            const int thumbY = juce::jlimit(y, y + height - thumbH, thumbCentreY - thumbH / 2);
            const int thumbX = x + (width - thumbW) / 2;

            g.setColour(Colours::white);
            g.drawImageAt(faderImg, thumbX, thumbY);
        }
        else {
            LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
        }


    }

    

    // Constrain JUCE's sliderPos calculation so the LinearVertical fader thumb
    // can't be placed at a Y that would draw the image past the slider bounds.
    // Without this the thumb's reachable extreme positions would let the bottom
    // edge of the fader graphic disappear below the slider's lower bound at
    // min value — even with the drawLinearSlider clamp, hit-testing/drag bounds
    // would still feel "off" because JUCE wouldn't know about our thumb size.
    int getSliderThumbRadius(juce::Slider& slider) override {
        if (slider.getSliderStyle() == juce::Slider::LinearVertical
            || slider.getSliderStyle() == juce::Slider::LinearHorizontal) {
            juce::Image fader = juce::ImageCache::getFromMemory(BinaryData::Fader_png, BinaryData::Fader_pngSize);
            // Displayed thumb is rescaled to half height; radius = half of that.
            return juce::jmax(6, fader.getHeight() / 4);
        }
        return juce::LookAndFeel_V4::getSliderThumbRadius(slider);
    }

    // Custom ComboBox Arrow
    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
        int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox& box) override
    {
        auto cornerSize = 6.0f;
        auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat();

        g.setColour(box.findColour(juce::ComboBox::backgroundColourId));
        g.fillRoundedRectangle(bounds, cornerSize);

        g.setColour(box.findColour(juce::ComboBox::outlineColourId));
        g.drawRoundedRectangle(bounds.reduced(0.5f, 0.5f), cornerSize, 1.0f);

        juce::Rectangle<int> arrowZone(width - 30, 0, 20, height);
        juce::Path path;
        path.startNewSubPath((float)arrowZone.getX() + 3.0f, (float)arrowZone.getCentreY() - 2.0f);
        path.lineTo((float)arrowZone.getCentreX(), (float)arrowZone.getCentreY() + 3.0f);
        path.lineTo((float)arrowZone.getRight() - 3.0f, (float)arrowZone.getCentreY() - 2.0f);

        g.setColour(box.findColour(juce::ComboBox::arrowColourId).withAlpha(box.isEnabled() ? 0.9f : 0.2f));
        g.strokePath(path, juce::PathStrokeType(2.0f));
    }

    // Custom ScrollBar
    void drawScrollbar(juce::Graphics& g, juce::ScrollBar& scrollbar, int x, int y, int width, int height,
        bool isScrollbarVertical, int thumbStartPosition, int thumbSize, bool isMouseOver, bool isMouseDown) override
    {
        juce::Rectangle<int> thumbBounds;

        if (isScrollbarVertical)
            thumbBounds = { x + 2, thumbStartPosition + 2, width - 4, thumbSize - 4 };
        else
            thumbBounds = { thumbStartPosition + 2, y + 2, thumbSize - 4, height - 4 };

        auto cornerSize = (float)juce::jmin(thumbBounds.getWidth(), thumbBounds.getHeight()) * 0.5f;

        g.setColour(scrollbar.findColour(juce::ScrollBar::thumbColourId).withAlpha(isMouseOver ? 0.8f : 0.6f));
        g.fillRoundedRectangle(thumbBounds.toFloat(), cornerSize);
    }

    // Bessere Fonts f�r bessere Lesbarkeit
    juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override
    {
        return juce::Font(juce::jmin(15.0f, (float)buttonHeight * 0.6f), juce::Font::FontStyleFlags::plain);
    }

    juce::Font getPopupMenuFont() override
    {
        return juce::Font(16.0f);
    }

    juce::Font getComboBoxFont(juce::ComboBox&) override
    {
        return juce::Font(14.0f);
    }

    juce::Font getLabelFont(juce::Label&) override
    {
        return juce::Font(16.0f);
    }

    // Besseres PopupMenu Styling
    void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override
    {
        g.fillAll(findColour(juce::PopupMenu::backgroundColourId));

        g.setColour(findColour(juce::PopupMenu::textColourId).withAlpha(0.1f));
        g.drawRect(0, 0, width, height);
    }

    void drawDocumentWindowTitleBar(juce::DocumentWindow& window,
        juce::Graphics& g,
        int w, int h,
        int titleSpaceX, int titleSpaceW,
        const juce::Image* icon,
        bool drawTitleTextOnLeft) override
    {
        // Hintergrund der Titelleiste zeichnen
        g.fillAll(juce::Colours::darkgrey.darker());

        // Optional: Titel-Text zeichnen
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(15.0f, juce::Font::bold));

        g.drawText(window.getName(),
            titleSpaceX, 0, titleSpaceW, h,
            juce::Justification::centred, true);
    }

    /*void drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
        bool isSeparator, bool isActive, bool isHighlighted, bool isTicked, bool hasSubMenu,
        const juce::String& text, const juce::String& shortcutKeyText,
        const juce::Drawable* icon, const juce::Colour* textColour) override
    {
        if (isSeparator)
        {
            auto r = area.reduced(5, 0);
            r.removeFromTop(r.getHeight() / 2);
            g.setColour(findColour(juce::PopupMenu::textColourId).withAlpha(0.3f));
            g.fillRect(r.removeFromTop(1));
        }
        else
        {
            auto textColourToUse = (textColour == nullptr ? findColour(juce::PopupMenu::textColourId) : *textColour);

            if (isHighlighted)
            {
                g.setColour(findColour(juce::PopupMenu::highlightedBackgroundColourId));
                g.fillRect(area.reduced(1));
                g.setColour(findColour(juce::PopupMenu::highlightedTextColourId));
            }
            else
            {
                g.setColour(textColourToUse);
            }

            if (!isActive)
                g.setOpacity(0.3f);

            juce::Font font(getPopupMenuFont());
            auto maxFontHeight = (float)area.getHeight() / 1.3f;

            if (font.getHeight() > maxFontHeight)
                font.setHeight(maxFontHeight);

            g.setFont(font);

            auto& iconArea = area.removeFromLeft(static_cast<int>(area.getHeight())).reduced(3).toFloat();

            if (icon != nullptr)
            {
                icon->drawWithin(g, iconArea, juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize, 1.0f);
            }
            else if (isTicked)
            {
                auto tick = getTickShape(1.0f);
                g.fillPath(tick, tick.getTransformToScaleToFit(iconArea.reduced(iconArea.getWidth() / 5, 0).toFloat(), true));
            }

            if (hasSubMenu)
            {
                auto arrowH = 0.6f * getPopupMenuFont().getAscent();
                auto x = static_cast<float>(area.getRight() - area.getHeight());
                auto halfH = static_cast<float>(area.getHeight()) * 0.5f;

                juce::Path path;
                path.startNewSubPath(x, halfH - arrowH * 0.5f);
                path.lineTo(x + arrowH * 0.6f, halfH);
                path.lineTo(x, halfH + arrowH * 0.5f);

                g.strokePath(path, juce::PathStrokeType(2.0f));
            }

            auto r = area.reduced(iconArea.getWidth(), 0);

            if (!shortcutKeyText.isEmpty())
            {
                auto f2 = font;
                f2.setHeight(f2.getHeight() * 0.75f);
                f2.setHorizontalScale(0.95f);
                g.setFont(f2);

                g.drawText(shortcutKeyText, r, juce::Justification::centredRight, true);
            }

            g.setFont(font);
            g.drawFittedText(text, r.reduced(2, 0), juce::Justification::centredLeft, 1);
        }
    }*/

    private:
        juce::ColourGradient gradientVertical;
        juce::ColourGradient gradientHorizontal;
        juce::ComponentAnimator animator;

        long triggerTime = 0;
        int maxY = 0xFFFF;
        int maxX = 0;
};
