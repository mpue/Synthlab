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
    // Moderne, leicht kühl getönte Dark-Palette. Statt der bisherigen flachen
    // Grautöne haben Hintergründe einen Hauch Blauton (#0e0f13/#16181d/…), so
    // wirkt die UI weniger nach Windows-95-Grau und mehr nach IDE/DAW von 2025.
    static constexpr juce::uint32 cBgWindow      = 0xff0e0f13;  // tiefster Hintergrund
    static constexpr juce::uint32 cBgPanel       = 0xff16181d;  // Panel/Tab-Inhalt
    static constexpr juce::uint32 cBgInput       = 0xff1d1f25;  // TextEditor/ComboBox/Button
    static constexpr juce::uint32 cBgInputHover  = 0xff262932;
    static constexpr juce::uint32 cBgRaised      = 0xff2a2d36;  // Tooltips, Popups
    static constexpr juce::uint32 cBorderSoft    = 0xff262932;  // ruhige Trennlinien
    static constexpr juce::uint32 cBorderHard    = 0xff353945;  // betonte Outlines
    static constexpr juce::uint32 cTextStrong    = 0xfff2f3f5;
    static constexpr juce::uint32 cText          = 0xffd6d8de;
    static constexpr juce::uint32 cTextMuted     = 0xff8a8e98;
    // Akzent: ein modernes Indigo (Tailwind-Indigo-400 ~ 0xff818cf8). Hebt sich
    // gut auf dunklem Grund ab, weniger „Standard-Blau"-Vibe.
    static constexpr juce::uint32 cAccent        = 0xff818cf8;
    static constexpr juce::uint32 cAccentBright  = 0xffa5b4fc;
    static constexpr juce::uint32 cAccentSoft    = 0x33818cf8;  // halbtransparent für Halos

    ElegantDarkLookAndFeel()
    {
        const juce::Colour bgWindow     (cBgWindow);
        const juce::Colour bgPanel      (cBgPanel);
        const juce::Colour bgInput      (cBgInput);
        const juce::Colour bgRaised     (cBgRaised);
        const juce::Colour borderSoft   (cBorderSoft);
        const juce::Colour borderHard   (cBorderHard);
        const juce::Colour text         (cText);
        const juce::Colour textStrong   (cTextStrong);
        const juce::Colour textMuted    (cTextMuted);
        const juce::Colour accent       (cAccent);
        const juce::Colour accentBright (cAccentBright);
        const juce::Colour accentSoft   (cAccentSoft);

        // Basis Farbschema
        setColour(juce::ResizableWindow::backgroundColourId, bgWindow);
        setColour(juce::DocumentWindow::backgroundColourId,  bgWindow);

        // Text Farben
        setColour(juce::Label::textColourId,                   text);
        setColour(juce::TextEditor::textColourId,              text);
        setColour(juce::TextEditor::backgroundColourId,        bgInput);
        setColour(juce::TextEditor::highlightColourId,         accentSoft);
        setColour(juce::TextEditor::outlineColourId,           borderSoft);
        setColour(juce::TextEditor::focusedOutlineColourId,    accent);

        // Button Farben
        setColour(juce::TextButton::buttonColourId,    bgInput);
        setColour(juce::TextButton::buttonOnColourId,  accent);
        setColour(juce::TextButton::textColourOffId,   text);
        setColour(juce::TextButton::textColourOnId,    juce::Colours::white);

        // ComboBox
        setColour(juce::ComboBox::backgroundColourId, bgInput);
        setColour(juce::ComboBox::textColourId,       text);
        setColour(juce::ComboBox::outlineColourId,    borderSoft);
        setColour(juce::ComboBox::buttonColourId,     borderHard);
        setColour(juce::ComboBox::arrowColourId,      textMuted);

        // PopupMenu
        setColour(juce::PopupMenu::backgroundColourId,            bgRaised);
        setColour(juce::PopupMenu::textColourId,                  text);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, accent);
        setColour(juce::PopupMenu::highlightedTextColourId,       juce::Colours::white);

        // Slider
        setColour(juce::Slider::backgroundColourId,           borderSoft);
        setColour(juce::Slider::thumbColourId,                accent);
        setColour(juce::Slider::trackColourId,                borderHard);
        setColour(juce::Slider::rotarySliderFillColourId,     accent);
        setColour(juce::Slider::rotarySliderOutlineColourId,  borderSoft);
        setColour(juce::Slider::textBoxTextColourId,          text);
        setColour(juce::Slider::textBoxBackgroundColourId,    bgInput);
        setColour(juce::Slider::textBoxOutlineColourId,       borderSoft);

        // ToggleButton
        setColour(juce::ToggleButton::textColourId,           text);
        setColour(juce::ToggleButton::tickColourId,           accent);
        setColour(juce::ToggleButton::tickDisabledColourId,   borderHard);

        // ListBox
        setColour(juce::ListBox::backgroundColourId, bgPanel);
        setColour(juce::ListBox::outlineColourId,    borderSoft);
        setColour(juce::ListBox::textColourId,       text);

        // TreeView
        setColour(juce::TreeView::backgroundColourId,             bgPanel);
        setColour(juce::TreeView::linesColourId,                  borderSoft);
        setColour(juce::TreeView::dragAndDropIndicatorColourId,   accent);
        setColour(juce::TreeView::selectedItemBackgroundColourId, accentSoft);

        // Scrollbar
        setColour(juce::ScrollBar::backgroundColourId, bgPanel);
        setColour(juce::ScrollBar::thumbColourId,      borderHard);
        setColour(juce::ScrollBar::trackColourId,      bgPanel);

        // TabbedComponent
        setColour(juce::TabbedComponent::backgroundColourId,     bgWindow);
        setColour(juce::TabbedComponent::outlineColourId,        borderSoft);
        setColour(juce::TabbedButtonBar::tabOutlineColourId,     juce::Colours::transparentBlack);
        setColour(juce::TabbedButtonBar::frontOutlineColourId,   accent);

        // TableListBox
        setColour(juce::TableListBox::backgroundColourId,           bgPanel);
        setColour(juce::TableListBox::outlineColourId,              borderSoft);
        setColour(juce::TableHeaderComponent::backgroundColourId,   bgInput);
        setColour(juce::TableHeaderComponent::outlineColourId,      borderSoft);
        setColour(juce::TableHeaderComponent::textColourId,         textStrong);
        setColour(juce::TableHeaderComponent::highlightColourId,    accent);

        // ProgressBar
        setColour(juce::ProgressBar::backgroundColourId, borderSoft);
        setColour(juce::ProgressBar::foregroundColourId, accent);

        // GroupComponent
        setColour(juce::GroupComponent::outlineColourId, borderSoft);
        setColour(juce::GroupComponent::textColourId,    textMuted);

        // DirectoryContentsDisplayComponent
        setColour(juce::DirectoryContentsDisplayComponent::highlightColourId, accentSoft);
        setColour(juce::DirectoryContentsDisplayComponent::textColourId,      text);

        // FileBrowserComponent
        setColour(juce::FileBrowserComponent::currentPathBoxBackgroundColourId, bgInput);
        setColour(juce::FileBrowserComponent::currentPathBoxTextColourId,       text);
        setColour(juce::FileBrowserComponent::currentPathBoxArrowColourId,      textMuted);
        setColour(juce::FileBrowserComponent::filenameBoxBackgroundColourId,    bgInput);
        setColour(juce::FileBrowserComponent::filenameBoxTextColourId,          text);

        // AlertWindow
        setColour(juce::AlertWindow::backgroundColourId, bgRaised);
        setColour(juce::AlertWindow::textColourId,       text);
        setColour(juce::AlertWindow::outlineColourId,    borderHard);

        // TooltipWindow
        setColour(juce::TooltipWindow::backgroundColourId, bgRaised);
        setColour(juce::TooltipWindow::textColourId,       text);
        setColour(juce::TooltipWindow::outlineColourId,    borderHard);

        // CodeEditorComponent
        setColour(juce::CodeEditorComponent::backgroundColourId,    bgWindow);
        setColour(juce::CodeEditorComponent::highlightColourId,     accentSoft);
        setColour(juce::CodeEditorComponent::defaultTextColourId,   text);
        setColour(juce::CodeEditorComponent::lineNumberBackgroundId, bgPanel);
        setColour(juce::CodeEditorComponent::lineNumberTextId,      textMuted);

        // CaretComponent
        setColour(juce::CaretComponent::caretColourId, accent);

        // HyperlinkButton
        setColour(juce::HyperlinkButton::textColourId, accentBright);

        // KeyboardComponentBase
        setColour(juce::KeyboardComponentBase::upDownButtonBackgroundColourId, bgInput);
        setColour(juce::KeyboardComponentBase::upDownButtonArrowColourId,      textMuted);

        // MidiKeyboardComponent
        setColour(juce::MidiKeyboardComponent::whiteNoteColourId,           textStrong);
        setColour(juce::MidiKeyboardComponent::blackNoteColourId,           bgWindow);
        setColour(juce::MidiKeyboardComponent::keySeparatorLineColourId,    borderSoft);
        setColour(juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, accentSoft);
        setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId,      accent.withAlpha(0.6f));
        setColour(juce::MidiKeyboardComponent::textLabelColourId,           bgWindow);
        setColour(juce::MidiKeyboardComponent::upDownButtonBackgroundColourId, bgInput);
        setColour(juce::MidiKeyboardComponent::upDownButtonArrowColourId,      textMuted);

        gradientVertical = juce::ColourGradient(juce::Colours::red, 0, 0, juce::Colours::chartreuse, 5, 128, false);
        gradientVertical.addColour(0.2, juce::Colours::lightgrey);

        gradientHorizontal = juce::ColourGradient(juce::Colours::chartreuse, 0, 0, juce::Colours::red, 128, 5, false);
        gradientHorizontal.addColour(0.8, juce::Colours::grey);

        // Default-Schriftart bewusst NICHT mehr hardcoden. JUCE nimmt dann den
        // System-Default — auf macOS SF Pro, auf Windows Segoe UI, auf Linux
        // den Distributions-Default. Das war der größte „altbacken"-Faktor:
        // Verdana ist seit ~2005 das Sinnbild für Web-1.0-Forensoftware.
    }

    // Modernes Button-Rendering: weicher Hover-Lift statt harter Outline,
    // dezenter Top-Highlight für „angehoben"-Effekt.
    void drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
        bool isMouseOverButton, bool isButtonDown) override
    {
        const auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
        const float cornerSize = juce::jmin(10.0f, bounds.getHeight() * 0.35f);

        const bool isToggled = button.getToggleState();
        const juce::Colour base =
            isToggled ? juce::Colour(cAccent)
                      : (isButtonDown   ? juce::Colour(cBgInput).brighter(0.10f)
                       : isMouseOverButton ? juce::Colour(cBgInputHover)
                                          : juce::Colour(cBgInput));

        // Soft drop shadow unter dem Button — nur im Default-State, damit der
        // gedrückte Zustand wirklich „eingedrückt" wirkt.
        if (!isButtonDown && !isToggled) {
            g.setColour(juce::Colours::black.withAlpha(0.35f));
            g.fillRoundedRectangle(bounds.translated(0.0f, 1.0f), cornerSize);
        }

        // Vertikaler Body-Gradient — minimal, sonst wirkt es plastisch.
        juce::ColourGradient body(base.brighter(0.04f), bounds.getX(), bounds.getY(),
                                   base.darker(0.08f),  bounds.getX(), bounds.getBottom(), false);
        g.setGradientFill(body);
        g.fillRoundedRectangle(bounds, cornerSize);

        // Subtiler Top-Highlight (1 px), kein Vollumriss.
        g.setColour(juce::Colours::white.withAlpha(isToggled ? 0.20f : 0.05f));
        juce::Path topLine;
        topLine.startNewSubPath(bounds.getX() + cornerSize, bounds.getY() + 0.5f);
        topLine.lineTo(bounds.getRight() - cornerSize, bounds.getY() + 0.5f);
        g.strokePath(topLine, juce::PathStrokeType(1.0f));

        // Hover: dezenter Halo-Glow rundherum (kein gradient blob mehr).
        if (isMouseOverButton && !isButtonDown && !isToggled) {
            g.setColour(juce::Colour(cAccent).withAlpha(0.18f));
            g.drawRoundedRectangle(bounds.expanded(0.5f), cornerSize, 1.2f);
        }
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

    // PopupMenu: leichter Drop-Shadow durch zwei konzentrische Rahmen, plus
    // gerundete Hintergrundfläche (statt einem rechteckigen Block mit harter
    // 1-px-Outline, was sehr nach Win-XP aussah).
    void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override
    {
        const auto bounds = juce::Rectangle<float>(0.0f, 0.0f, (float)width, (float)height);
        const float corner = 8.0f;

        // Outer „shadow" stripe (nur sichtbar wenn das Popup nicht am Bildrand klebt).
        g.setColour(juce::Colours::black.withAlpha(0.55f));
        g.fillRoundedRectangle(bounds, corner);

        // Eigentliche Fläche, leicht eingerückt — kein knallharter Vollrand.
        g.setColour(findColour(juce::PopupMenu::backgroundColourId));
        g.fillRoundedRectangle(bounds.reduced(1.0f), corner - 1.0f);

        // Sehr dünne, kühle Innenkante.
        g.setColour(juce::Colour(cBorderHard).withAlpha(0.6f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), corner, 0.8f);
    }

    void drawDocumentWindowTitleBar(juce::DocumentWindow& window,
        juce::Graphics& g,
        int w, int h,
        int titleSpaceX, int titleSpaceW,
        const juce::Image* icon,
        bool drawTitleTextOnLeft) override
    {
        // Vertikaler Verlauf statt flachem Mittelgrau — wirkt deutlich
        // hochwertiger und passt zur abgesetzten Window-Background-Farbe.
        const auto bounds = juce::Rectangle<float>(0.0f, 0.0f, (float)w, (float)h);
        juce::ColourGradient grad(juce::Colour(cBgRaised),  bounds.getX(), bounds.getY(),
                                   juce::Colour(cBgPanel),   bounds.getX(), bounds.getBottom(), false);
        g.setGradientFill(grad);
        g.fillRect(bounds);

        // Dünne Trennlinie zur Inhaltsfläche.
        g.setColour(juce::Colour(cBorderSoft));
        g.fillRect(0, h - 1, w, 1);

        // Titel: Medium-weight, leicht ausgegraut, nicht aufdringlich.
        g.setColour(juce::Colour(cTextStrong));
        g.setFont(juce::Font(14.0f, juce::Font::FontStyleFlags::plain));
        g.drawText(window.getName(),
            titleSpaceX, 0, titleSpaceW, h,
            juce::Justification::centred, true);
    }

    // TabBar: Underline-Style. Aktiver Tab kriegt eine 2-px-Akzent-Linie
    // unten (wie in Linear/Vercel/VSCode), kein Box-Outline mehr.
    void drawTabButton(juce::TabBarButton& button, juce::Graphics& g,
                       bool isMouseOver, bool isMouseDown) override
    {
        const auto area = button.getActiveArea().toFloat();
        const bool isActive = button.isFrontTab();

        // Hover-/Aktiv-Hintergrund.
        if (isActive)
        {
            g.setColour(juce::Colour(cBgPanel));
            g.fillRect(area);
        }
        else if (isMouseOver)
        {
            g.setColour(juce::Colour(cBgInputHover).withAlpha(0.5f));
            g.fillRect(area);
        }

        // Underline für aktiven Tab.
        if (isActive)
        {
            g.setColour(juce::Colour(cAccent));
            g.fillRect(area.getX(), area.getBottom() - 2.0f, area.getWidth(), 2.0f);
        }

        // Text.
        g.setColour(isActive ? juce::Colour(cTextStrong)
                             : juce::Colour(cTextMuted).withAlpha(isMouseOver ? 0.95f : 0.7f));
        g.setFont(juce::Font(13.0f, isActive ? juce::Font::bold : juce::Font::plain));
        g.drawText(button.getButtonText(), area.toNearestInt(), juce::Justification::centred, false);
    }

    // TextEditor-Outline: Default ist eine harte 1-px-Box. Wir machen sie
    // dünner und im Focused-State doppelt — innen subtil, außen Akzent.
    void drawTextEditorOutline(juce::Graphics& g, int width, int height,
                               juce::TextEditor& editor) override
    {
        if (editor.isReadOnly() || !editor.isEnabled())
            return;

        const auto bounds = juce::Rectangle<float>(0.0f, 0.0f, (float)width, (float)height).reduced(0.5f);

        if (editor.hasKeyboardFocus(true))
        {
            // Akzent-Border + dezenter Halo außenrum.
            g.setColour(juce::Colour(cAccent).withAlpha(0.25f));
            g.drawRoundedRectangle(bounds.expanded(1.0f), 5.0f, 2.0f);

            g.setColour(juce::Colour(cAccent));
            g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
        }
        else
        {
            g.setColour(juce::Colour(cBorderSoft));
            g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
        }
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
