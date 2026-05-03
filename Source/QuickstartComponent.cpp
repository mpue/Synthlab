/*
  ==============================================================================

    QuickstartComponent.cpp

  ==============================================================================
*/

#include "QuickstartComponent.h"

namespace
{
    constexpr int kMaxContentWidth   = 760;
    constexpr int kHeaderHeight      = 160;
    constexpr int kCardHeight        = 128;
    constexpr int kCardSpacing       = 18;
    constexpr int kCardCornerRadius  = 14;
    constexpr int kFooterHeight      = 56;
    constexpr int kHorizontalPadding = 32;

    const juce::Colour kBgColour          { 0xff1b1b25 };
    const juce::Colour kCardColour        { 0xff2a2a3a };
    const juce::Colour kCardHoverColour   { 0xff32324a };
    const juce::Colour kCardBorderColour  { 0x33ffffff };
    const juce::Colour kAccentColour      { 0xff6366f1 }; // Indigo
    const juce::Colour kAccentSecondary   { 0xff8b5cf6 }; // Violet
    const juce::Colour kAccentSuccess     { 0xff10b981 }; // Emerald
    const juce::Colour kTextPrimary       { 0xfff1f5f9 };
    const juce::Colour kTextSecondary     { 0xffa0a9c0 };
    const juce::Colour kTextMuted         { 0xff7a839d };

    juce::File quickstartPropsFile()
    {
        const auto home = juce::File::getSpecialLocation(juce::File::userHomeDirectory);
        const auto appDir = home.getChildFile(".Synthlab");
        if (! appDir.exists())
            appDir.createDirectory();
        return appDir.getChildFile("quickstart.properties");
    }
}

bool QuickstartComponent::getShowOnStartup()
{
    juce::PropertiesFile::Options options;
    juce::PropertiesFile props(quickstartPropsFile(), options);
    // Default true — first-run users should see the wizard.
    return props.getBoolValue("showOnStartup", true);
}

void QuickstartComponent::setShowOnStartup(bool show)
{
    juce::PropertiesFile::Options options;
    juce::PropertiesFile props(quickstartPropsFile(), options);
    props.setValue("showOnStartup", show);
    props.saveIfNeeded();
}

QuickstartComponent::QuickstartComponent()
{
    steps[0].number      = 1;
    steps[0].title       = "Configure your audio interface";
    steps[0].description = "Pick your input/output device, sample rate and buffer size so Synthlab can talk to your hardware.";
    steps[0].actionLabel = "Open Audio Settings";

    steps[1].number      = 2;
    steps[1].title       = "Scan your plugins";
    steps[1].description = "Discover the VST/AU instruments and effects installed on your system. You only need to do this once.";
    steps[1].actionLabel = "Scan Plugins";

    steps[2].number      = 3;
    steps[2].title       = "Create an instrument track";
    steps[2].description = "Add a virtual instrument and start playing. Pick from your scanned plugins or one of the built-in synths.";
    steps[2].actionLabel = "Add Instrument Track";

    steps[3].number      = 4;
    steps[3].title       = "Open an existing track";
    steps[3].description = "Pick a previously saved Synthlab project and continue where you left off.";
    steps[3].actionLabel = "Open Existing Track";

    for (auto& step : steps)
    {
        step.button = std::make_unique<juce::TextButton>(step.actionLabel);
        step.button->setColour(juce::TextButton::buttonColourId,   kAccentColour);
        step.button->setColour(juce::TextButton::buttonOnColourId, kAccentSecondary);
        step.button->setColour(juce::TextButton::textColourOffId,  juce::Colours::white);
        step.button->setColour(juce::TextButton::textColourOnId,   juce::Colours::white);
        step.button->addListener(this);
        addAndMakeVisible(step.button.get());
    }

    showOnStartupToggle = std::make_unique<juce::ToggleButton>("Show this Quickstart on startup");
    showOnStartupToggle->setColour(juce::ToggleButton::textColourId,        kTextSecondary);
    showOnStartupToggle->setColour(juce::ToggleButton::tickColourId,        kAccentColour);
    showOnStartupToggle->setColour(juce::ToggleButton::tickDisabledColourId, kCardBorderColour);
    showOnStartupToggle->setToggleState(getShowOnStartup(), juce::dontSendNotification);
    showOnStartupToggle->addListener(this);
    addAndMakeVisible(showOnStartupToggle.get());

    setOpaque(true);
    startTimer(750); // refresh status indicators
}

QuickstartComponent::~QuickstartComponent()
{
    stopTimer();
    for (auto& step : steps)
    {
        if (step.button != nullptr)
            step.button->removeListener(this);
    }
    if (showOnStartupToggle != nullptr)
        showOnStartupToggle->removeListener(this);
}

void QuickstartComponent::resized()
{
    layoutSteps();
}

void QuickstartComponent::layoutSteps()
{
    auto bounds = getLocalBounds();

    const int contentWidth = juce::jmin(bounds.getWidth() - 2 * kHorizontalPadding, kMaxContentWidth);
    const int contentX = bounds.getX() + (bounds.getWidth() - contentWidth) / 2;

    headerBounds = juce::Rectangle<int>(contentX, bounds.getY() + 28, contentWidth, kHeaderHeight);

    int y = headerBounds.getBottom() + 24;
    contentBounds = juce::Rectangle<int>(contentX, y, contentWidth, (int)steps.size() * (kCardHeight + kCardSpacing));

    for (auto& step : steps)
    {
        step.bounds = juce::Rectangle<int>(contentX, y, contentWidth, kCardHeight);

        if (step.button != nullptr)
        {
            const int btnW = 200;
            const int btnH = 38;
            step.button->setBounds(step.bounds.getRight() - btnW - 24,
                                   step.bounds.getCentreY() - btnH / 2,
                                   btnW, btnH);
        }

        y += kCardHeight + kCardSpacing;
    }

    footerBounds = juce::Rectangle<int>(contentX, y + 8, contentWidth, kFooterHeight);

    if (showOnStartupToggle != nullptr)
        showOnStartupToggle->setBounds(footerBounds.reduced(4));
}

void QuickstartComponent::paint(juce::Graphics& g)
{
    g.fillAll(kBgColour);

    // Subtle radial vignette behind the hero header to give the page some depth.
    {
        juce::ColourGradient bgGlow(
            kAccentColour.withAlpha(0.18f),
            (float)getWidth() * 0.5f, -120.0f,
            kBgColour.withAlpha(0.0f),
            (float)getWidth() * 0.5f, (float)getHeight() * 0.55f,
            true);
        g.setGradientFill(bgGlow);
        g.fillAll();
    }

    paintHeader(g, headerBounds);

    for (const auto& step : steps)
        paintStep(g, step);
}

void QuickstartComponent::paintHeader(juce::Graphics& g, juce::Rectangle<int> area)
{
    if (area.isEmpty())
        return;

    juce::Path hero;
    hero.addRoundedRectangle(area.toFloat(), (float)kCardCornerRadius * 1.4f);

    juce::ColourGradient grad(
        kAccentColour, (float)area.getX(), (float)area.getY(),
        kAccentSecondary, (float)area.getRight(), (float)area.getBottom(),
        false);
    grad.addColour(0.55, juce::Colour(0xff5b21b6));

    g.setGradientFill(grad);
    g.fillPath(hero);

    // Soft inner highlight along the top edge for a glassy feel.
    g.setColour(juce::Colours::white.withAlpha(0.08f));
    g.drawLine((float)area.getX() + 16, (float)area.getY() + 1,
               (float)area.getRight() - 16, (float)area.getY() + 1, 1.0f);

    auto inner = area.reduced(28, 22);

    g.setColour(juce::Colours::white.withAlpha(0.85f));
    g.setFont(juce::Font(13.0f, juce::Font::bold));
    g.drawText("QUICKSTART", inner.removeFromTop(18), juce::Justification::topLeft, false);

    inner.removeFromTop(4);

    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(30.0f, juce::Font::bold));
    g.drawText("Welcome to Synthlab", inner.removeFromTop(40), juce::Justification::topLeft, false);

    g.setColour(juce::Colours::white.withAlpha(0.85f));
    g.setFont(juce::Font(15.0f));
    g.drawFittedText(
        "Three quick steps and you're ready to make sound. "
        "You can come back to this page from the menu at any time.",
        inner.removeFromTop(46), juce::Justification::topLeft, 2);
}

void QuickstartComponent::paintStep(juce::Graphics& g, const StepCard& step)
{
    if (step.bounds.isEmpty())
        return;

    auto bounds = step.bounds.toFloat();

    // Card with subtle drop shadow.
    {
        juce::DropShadow shadow(juce::Colours::black.withAlpha(0.35f), 12, { 0, 4 });
        juce::Path p;
        p.addRoundedRectangle(bounds, (float)kCardCornerRadius);
        shadow.drawForPath(g, p);
    }

    g.setColour(kCardColour);
    g.fillRoundedRectangle(bounds, (float)kCardCornerRadius);

    // Accent stripe on the left edge — coloured green when the step is done.
    {
        const float stripeW = 4.0f;
        juce::Path stripe;
        stripe.addRoundedRectangle(bounds.getX(), bounds.getY(),
                                   stripeW + (float)kCardCornerRadius,
                                   bounds.getHeight(),
                                   (float)kCardCornerRadius, (float)kCardCornerRadius,
                                   true, false, true, false);
        g.setColour(step.done ? kAccentSuccess : kAccentColour);
        g.fillPath(stripe);
    }

    // Hairline border.
    g.setColour(kCardBorderColour);
    g.drawRoundedRectangle(bounds, (float)kCardCornerRadius, 1.0f);

    // ---- Step number circle -------------------------------------------------
    const float badgeSize = 56.0f;
    const float badgeX = bounds.getX() + 28.0f;
    const float badgeY = bounds.getCentreY() - badgeSize / 2.0f;

    {
        juce::ColourGradient badgeGrad(
            step.done ? kAccentSuccess.brighter(0.15f) : kAccentColour.brighter(0.1f),
            badgeX, badgeY,
            step.done ? kAccentSuccess.darker(0.2f)    : kAccentSecondary,
            badgeX, badgeY + badgeSize,
            false);
        g.setGradientFill(badgeGrad);
        g.fillEllipse(badgeX, badgeY, badgeSize, badgeSize);
    }

    g.setColour(juce::Colours::white.withAlpha(0.18f));
    g.drawEllipse(badgeX, badgeY, badgeSize, badgeSize, 1.5f);

    if (step.done)
    {
        // Draw a check mark.
        juce::Path check;
        const float cx = badgeX + badgeSize * 0.5f;
        const float cy = badgeY + badgeSize * 0.5f;
        check.startNewSubPath(cx - 11.0f, cy + 1.0f);
        check.lineTo        (cx -  3.0f, cy + 9.0f);
        check.lineTo        (cx + 12.0f, cy - 8.0f);
        g.setColour(juce::Colours::white);
        g.strokePath(check, juce::PathStrokeType(3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    else
    {
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(26.0f, juce::Font::bold));
        g.drawText(juce::String(step.number),
                   juce::Rectangle<float>(badgeX, badgeY, badgeSize, badgeSize),
                   juce::Justification::centred);
    }

    // ---- Text block ---------------------------------------------------------
    const int textLeft = (int)(badgeX + badgeSize + 22);
    const int textRight = (step.button != nullptr ? step.button->getX() : (int)bounds.getRight()) - 16;
    const int textWidth = juce::jmax(120, textRight - textLeft);

    auto textArea = juce::Rectangle<int>(textLeft, (int)bounds.getY() + 22, textWidth, (int)bounds.getHeight() - 44);

    g.setColour(kTextPrimary);
    g.setFont(juce::Font(17.0f, juce::Font::bold));
    g.drawText(step.title, textArea.removeFromTop(22), juce::Justification::centredLeft, true);

    textArea.removeFromTop(4);

    g.setColour(kTextSecondary);
    g.setFont(juce::Font(13.5f));
    g.drawFittedText(step.description, textArea, juce::Justification::topLeft, 3);

    // Der visuelle Status wird ausschliesslich ueber den gruenen Akzent-
    // Streifen am linken Kartenrand und das Check-Mark im Badge ausgedrueckt
    // — keine zusaetzliche "DONE"-Beschriftung neben dem Button.
}

void QuickstartComponent::buttonClicked(juce::Button* b)
{
    if (b == showOnStartupToggle.get())
    {
        setShowOnStartup(showOnStartupToggle->getToggleState());
        return;
    }

    for (size_t i = 0; i < steps.size(); ++i)
    {
        if (b == steps[i].button.get())
        {
            switch (i)
            {
                case 0: if (onConfigureAudio)        onConfigureAudio();        break;
                case 1: if (onScanPlugins)           onScanPlugins();           break;
                case 2: if (onCreateInstrumentTrack) onCreateInstrumentTrack(); break;
                case 3: if (onOpenExistingTrack)     onOpenExistingTrack();     break;
                default: break;
            }
            return;
        }
    }
}

void QuickstartComponent::timerCallback()
{
    bool changed = false;

    auto refresh = [&changed](StepCard& step, const StatusProvider& provider)
    {
        const bool done = provider ? provider() : false;
        if (done != step.done)
        {
            step.done = done;
            changed = true;
        }
    };

    refresh(steps[0], isAudioConfigured);
    refresh(steps[1], isPluginsScanned);
    refresh(steps[2], hasInstrumentTrack);
    refresh(steps[3], hasOpenedExistingTrack);

    if (changed)
        repaint();
}
