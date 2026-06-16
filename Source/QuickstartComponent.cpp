/*
  ==============================================================================

    QuickstartComponent.cpp

  ==============================================================================
*/

#include "QuickstartComponent.h"

namespace
{
    constexpr int kMaxContentWidth   = 760;
    constexpr int kStepsColumnWidth  = 520; // smaler Linke-Spalte, damit rechts die Recent-Liste passt
    constexpr int kRecentsWidth      = 280;
    constexpr int kColumnGap         = 24;
    constexpr int kHeaderHeight      = 168;
    constexpr int kCardHeight        = 132;
    constexpr int kCardSpacing       = 18;
    constexpr int kCardCornerRadius  = 14;
    constexpr int kFooterHeight      = 56;
    constexpr int kHorizontalPadding = 32;
    constexpr int kTopPadding        = 28;
    constexpr int kBottomPadding     = 28;
    constexpr int kRecentEntryHeight = 52;
    constexpr int kRecentEntrySpacing = 6;
    constexpr int kRecentHeaderHeight = 32;
    constexpr int kRecentRowsStacked  = 6;  // Anzahl Recent-Slots in der Ein-Spalten-Ansicht

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

//==============================================================================
// Inner content — all painting/layout. Sized to its full required height by
// the outer component so the Viewport can scroll it.
//==============================================================================
class QuickstartComponent::Content : public juce::Component,
                                      private juce::Timer,
                                      private juce::Button::Listener
{
public:
    Content();
    ~Content() override;

    // Height needed to lay everything out at the given width (single- vs
    // two-column depends on width). Used by the outer component to size us
    // before handing us to the Viewport.
    int idealHeightForWidth(int width) const;

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseMove(const juce::MouseEvent&) override;
    void mouseExit(const juce::MouseEvent&) override;
    void mouseDown(const juce::MouseEvent&) override;

    ActionCallback onConfigureAudio;
    ActionCallback onScanPlugins;
    ActionCallback onCreateInstrumentTrack;
    ActionCallback onOpenExistingTrack;
    PathCallback   onOpenRecentFile;
    RecentProvider recentFilesProvider;

    StatusProvider isAudioConfigured;
    StatusProvider isPluginsScanned;
    StatusProvider hasInstrumentTrack;
    StatusProvider hasOpenedExistingTrack;

private:
    void timerCallback() override;
    void buttonClicked(juce::Button*) override;

    struct StepCard
    {
        int number = 0;
        juce::String title;
        juce::String description;
        juce::String actionLabel;
        juce::Rectangle<int> bounds;
        std::unique_ptr<juce::TextButton> button;
        bool done = false;
    };

    void layoutSteps();
    void paintHeader(juce::Graphics&, juce::Rectangle<int> area);
    void paintStep(juce::Graphics&, const StepCard& step);
    void paintRecents(juce::Graphics&);
    void refreshRecentEntries();
    int  hitTestRecentEntry(juce::Point<int> pos) const;
    static bool isSideBySide(int width);

    struct RecentEntry
    {
        juce::String fullPath;
        juce::String displayName;
        juce::String parentDir;
        juce::Rectangle<int> bounds;
    };

    std::array<StepCard, 4> steps;
    std::unique_ptr<juce::ToggleButton> showOnStartupToggle;

    std::vector<RecentEntry> recentEntries;
    juce::Rectangle<int>     recentBounds;
    int                      recentHoverIndex = -1;

    juce::Rectangle<int> headerBounds;
    juce::Rectangle<int> contentBounds;
    juce::Rectangle<int> footerBounds;
};

//==============================================================================
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

//==============================================================================
QuickstartComponent::Content::Content()
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

QuickstartComponent::Content::~Content()
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

bool QuickstartComponent::Content::isSideBySide(int width)
{
    const int columnsTotal = kStepsColumnWidth + kColumnGap + kRecentsWidth;
    return width >= columnsTotal + 2 * kHorizontalPadding;
}

int QuickstartComponent::Content::idealHeightForWidth(int width) const
{
    int y = kTopPadding + kHeaderHeight + 24;
    y += (int) steps.size() * (kCardHeight + kCardSpacing);
    y += 8 + kFooterHeight;

    if (! isSideBySide(width))
    {
        // Recent-Files-Box wird unter den Footer gestapelt.
        const int recentsH = kRecentRowsStacked * (kRecentEntryHeight + kRecentEntrySpacing)
                             + kRecentHeaderHeight + 24;
        y += 16 + recentsH;
    }

    return y + kBottomPadding;
}

void QuickstartComponent::Content::resized()
{
    layoutSteps();
}

void QuickstartComponent::Content::layoutSteps()
{
    auto bounds = getLocalBounds();

    // Zwei Spalten: links die Steps, rechts (wenn genug Platz) die Recent-
    // Files-Liste. Bei schmalem Fenster wandert die Recent-Box unter die Steps.
    const int columnsTotal = kStepsColumnWidth + kColumnGap + kRecentsWidth;
    const bool sideBySide = isSideBySide(bounds.getWidth());

    int stepsWidth, contentX;
    if (sideBySide) {
        stepsWidth = kStepsColumnWidth;
        contentX = bounds.getX() + (bounds.getWidth() - columnsTotal) / 2;
    } else {
        stepsWidth = juce::jmin(bounds.getWidth() - 2 * kHorizontalPadding, kMaxContentWidth);
        contentX = bounds.getX() + (bounds.getWidth() - stepsWidth) / 2;
    }

    // Header zieht sich ueber beide Spalten — sieht im "schick" besser aus.
    const int headerWidth = sideBySide ? columnsTotal : stepsWidth;
    headerBounds = juce::Rectangle<int>(contentX, bounds.getY() + kTopPadding, headerWidth, kHeaderHeight);

    int y = headerBounds.getBottom() + 24;
    contentBounds = juce::Rectangle<int>(contentX, y, stepsWidth, (int)steps.size() * (kCardHeight + kCardSpacing));

    for (auto& step : steps)
    {
        step.bounds = juce::Rectangle<int>(contentX, y, stepsWidth, kCardHeight);

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

    footerBounds = juce::Rectangle<int>(contentX, y + 8, stepsWidth, kFooterHeight);
    if (showOnStartupToggle != nullptr)
        showOnStartupToggle->setBounds(footerBounds.reduced(4));

    // Rechte Spalte: Recent-Files-Box. Hoehe = volle Steps-Spalten-Hoehe damit
    // die Karten optisch zueinander stehen. Wenn nicht side-by-side, unter
    // dem Footer eine kompakte Variante mit fester Slot-Zahl.
    if (sideBySide) {
        recentBounds = juce::Rectangle<int>(contentBounds.getRight() + kColumnGap,
                                            contentBounds.getY(),
                                            kRecentsWidth,
                                            contentBounds.getHeight());
    } else {
        recentBounds = juce::Rectangle<int>(contentX,
                                            footerBounds.getBottom() + 16,
                                            stepsWidth,
                                            kRecentRowsStacked * (kRecentEntryHeight + kRecentEntrySpacing)
                                                + kRecentHeaderHeight + 24);
    }

    refreshRecentEntries();
}

void QuickstartComponent::Content::paint(juce::Graphics& g)
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

    paintRecents(g);
}

void QuickstartComponent::Content::refreshRecentEntries()
{
    recentEntries.clear();

    juce::StringArray paths;
    if (recentFilesProvider) paths = recentFilesProvider();

    // Im Project-Modell ist der frischeste Eintrag hinten — wir wollen ihn
    // oben anzeigen, also rueckwaerts iterieren.
    int x = recentBounds.getX() + 14;
    int y = recentBounds.getY() + kRecentHeaderHeight + 8;
    const int w = recentBounds.getWidth() - 28;

    const int maxEntries = juce::jmax(0, (recentBounds.getHeight() - kRecentHeaderHeight - 24)
                                          / (kRecentEntryHeight + kRecentEntrySpacing));
    int placed = 0;
    for (int i = paths.size() - 1; i >= 0 && placed < maxEntries; --i)
    {
        const auto& full = paths[i];
        if (full.isEmpty()) continue;
        juce::File f (full);
        RecentEntry e;
        e.fullPath    = full;
        e.displayName = f.getFileNameWithoutExtension();
        e.parentDir   = f.getParentDirectory().getFileName();
        e.bounds      = juce::Rectangle<int>(x, y, w, kRecentEntryHeight);
        recentEntries.push_back(std::move(e));

        y += kRecentEntryHeight + kRecentEntrySpacing;
        ++placed;
    }
}

void QuickstartComponent::Content::paintRecents(juce::Graphics& g)
{
    if (recentBounds.isEmpty()) return;

    // Karten-Hintergrund.
    juce::Path bg;
    bg.addRoundedRectangle(recentBounds.toFloat(), (float) kCardCornerRadius);
    g.setColour(kCardColour);
    g.fillPath(bg);
    g.setColour(kCardBorderColour);
    g.strokePath(bg, juce::PathStrokeType(1.0f));

    // Akzent-Streifen oben.
    {
        juce::Path stripe;
        stripe.addRoundedRectangle((float) recentBounds.getX(),
                                   (float) recentBounds.getY(),
                                   (float) recentBounds.getWidth(),
                                   4.0f,
                                   2.0f);
        juce::ColourGradient grad(kAccentColour, (float) recentBounds.getX(), 0.0f,
                                  kAccentSecondary, (float) recentBounds.getRight(), 0.0f, false);
        g.setGradientFill(grad);
        g.fillPath(stripe);
    }

    // Header.
    g.setColour(kTextPrimary);
    g.setFont(juce::Font(15.0f, juce::Font::bold));
    g.drawText("Recent files",
               recentBounds.getX() + 14,
               recentBounds.getY() + 8,
               recentBounds.getWidth() - 28,
               kRecentHeaderHeight - 4,
               juce::Justification::centredLeft, false);

    if (recentEntries.empty())
    {
        g.setColour(kTextMuted);
        g.setFont(juce::Font(13.5f));
        g.drawText("No recent files yet.",
                   recentBounds.reduced(14, kRecentHeaderHeight + 12),
                   juce::Justification::topLeft, false);
        return;
    }

    for (size_t i = 0; i < recentEntries.size(); ++i)
    {
        const auto& e = recentEntries[i];
        const bool hover = ((int) i == recentHoverIndex);

        // Item-Hintergrund.
        juce::Path itemBg;
        itemBg.addRoundedRectangle(e.bounds.toFloat(), 8.0f);
        g.setColour(hover ? kCardHoverColour : kBgColour.brighter(0.04f));
        g.fillPath(itemBg);
        g.setColour(hover ? kAccentColour.withAlpha(0.55f) : kCardBorderColour);
        g.strokePath(itemBg, juce::PathStrokeType(1.0f));

        // Filename.
        g.setColour(kTextPrimary);
        g.setFont(juce::Font(14.0f, juce::Font::bold));
        g.drawText(e.displayName,
                   e.bounds.getX() + 12,
                   e.bounds.getY() + 6,
                   e.bounds.getWidth() - 24,
                   18,
                   juce::Justification::centredLeft, true);

        // Parent dir als Sub-Label.
        g.setColour(kTextSecondary);
        g.setFont(juce::Font(12.0f));
        g.drawText(e.parentDir,
                   e.bounds.getX() + 12,
                   e.bounds.getY() + 26,
                   e.bounds.getWidth() - 24,
                   16,
                   juce::Justification::centredLeft, true);

        if (hover)
        {
            g.setColour(kAccentColour);
            g.setFont(juce::Font(11.0f, juce::Font::bold));
            g.drawText("OPEN",
                       e.bounds.getRight() - 50,
                       e.bounds.getY() + 6,
                       40, 18,
                       juce::Justification::centredRight, false);
        }
    }
}

int QuickstartComponent::Content::hitTestRecentEntry(juce::Point<int> pos) const
{
    for (size_t i = 0; i < recentEntries.size(); ++i)
        if (recentEntries[i].bounds.contains(pos))
            return (int) i;
    return -1;
}

void QuickstartComponent::Content::mouseMove(const juce::MouseEvent& e)
{
    const int idx = hitTestRecentEntry(e.getPosition());
    setMouseCursor(idx >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
    if (idx != recentHoverIndex)
    {
        recentHoverIndex = idx;
        repaint(recentBounds);
    }
}

void QuickstartComponent::Content::mouseExit(const juce::MouseEvent&)
{
    if (recentHoverIndex != -1)
    {
        recentHoverIndex = -1;
        repaint(recentBounds);
    }
}

void QuickstartComponent::Content::mouseDown(const juce::MouseEvent& e)
{
    const int idx = hitTestRecentEntry(e.getPosition());
    if (idx < 0 || idx >= (int) recentEntries.size()) return;
    if (onOpenRecentFile)
        onOpenRecentFile(recentEntries[(size_t) idx].fullPath);
}

void QuickstartComponent::Content::paintHeader(juce::Graphics& g, juce::Rectangle<int> area)
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

    auto inner = area.reduced(28, 24);

    g.setColour(juce::Colours::white.withAlpha(0.85f));
    g.setFont(juce::Font(13.0f, juce::Font::bold));
    g.drawText("QUICKSTART", inner.removeFromTop(18), juce::Justification::topLeft, false);

    inner.removeFromTop(6);

    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(30.0f, juce::Font::bold));
    g.drawText("Welcome to Synthlab", inner.removeFromTop(40), juce::Justification::topLeft, false);

    inner.removeFromTop(6);

    g.setColour(juce::Colours::white.withAlpha(0.85f));
    g.setFont(juce::Font(15.0f));
    g.drawFittedText(
        "Three quick steps and you're ready to make sound. "
        "You can come back to this page from the menu at any time.",
        inner, juce::Justification::topLeft, 3);
}

void QuickstartComponent::Content::paintStep(juce::Graphics& g, const StepCard& step)
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
    g.setFont(juce::Font(18.0f, juce::Font::bold));
    g.drawText(step.title, textArea.removeFromTop(24), juce::Justification::centredLeft, true);

    textArea.removeFromTop(4);

    g.setColour(kTextSecondary);
    g.setFont(juce::Font(14.0f));
    g.drawFittedText(step.description, textArea, juce::Justification::topLeft, 3);

    // Der visuelle Status wird ausschliesslich ueber den gruenen Akzent-
    // Streifen am linken Kartenrand und das Check-Mark im Badge ausgedrueckt
    // — keine zusaetzliche "DONE"-Beschriftung neben dem Button.
}

void QuickstartComponent::Content::buttonClicked(juce::Button* b)
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

void QuickstartComponent::Content::timerCallback()
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

    // Recent-Liste auch refreshen — pruefen ob sich Inhalt veraendert hat.
    const size_t before = recentEntries.size();
    juce::String firstBefore = before > 0 ? recentEntries[0].fullPath : juce::String();
    refreshRecentEntries();
    const size_t after = recentEntries.size();
    juce::String firstAfter = after > 0 ? recentEntries[0].fullPath : juce::String();
    if (before != after || firstBefore != firstAfter)
        changed = true;

    if (changed)
        repaint();
}

//==============================================================================
// Outer wrapper — owns the Viewport and sizes the content to its full height.
//==============================================================================
QuickstartComponent::QuickstartComponent()
{
    content = std::make_unique<Content>();

    viewport.setViewedComponent(content.get(), false);
    viewport.setScrollBarsShown(true, false); // vertical only
    viewport.setScrollOnDragMode(juce::Viewport::ScrollOnDragMode::nonHover);
    addAndMakeVisible(viewport);

    setOpaque(true);
}

QuickstartComponent::~QuickstartComponent() = default;

void QuickstartComponent::paint(juce::Graphics& g)
{
    g.fillAll(kBgColour);
}

void QuickstartComponent::resized()
{
    viewport.setBounds(getLocalBounds());

    int width = getWidth();
    int height = content->idealHeightForWidth(width);

    // Wenn der Inhalt hoeher ist als sichtbar, erscheint die vertikale
    // Scrollbar — also Breite um deren Dicke reduzieren und neu rechnen.
    if (height > getHeight())
    {
        width = juce::jmax(0, width - viewport.getScrollBarThickness());
        height = content->idealHeightForWidth(width);
    }

    // Mindestens die Viewport-Hoehe ausfuellen, damit kein Streifen
    // Hintergrund unter dem Inhalt aufblitzt.
    height = juce::jmax(height, getHeight());

    content->setSize(width, height);
}

// ---- Forwarders to the inner content ---------------------------------------
void QuickstartComponent::setOnConfigureAudio(ActionCallback cb)        { content->onConfigureAudio = std::move(cb); }
void QuickstartComponent::setOnScanPlugins(ActionCallback cb)           { content->onScanPlugins = std::move(cb); }
void QuickstartComponent::setOnCreateInstrumentTrack(ActionCallback cb) { content->onCreateInstrumentTrack = std::move(cb); }
void QuickstartComponent::setOnOpenExistingTrack(ActionCallback cb)     { content->onOpenExistingTrack = std::move(cb); }
void QuickstartComponent::setOnOpenRecentFile(PathCallback cb)          { content->onOpenRecentFile = std::move(cb); }
void QuickstartComponent::setRecentFilesProvider(RecentProvider p)      { content->recentFilesProvider = std::move(p); }

void QuickstartComponent::setAudioConfiguredProvider(StatusProvider p)     { content->isAudioConfigured = std::move(p); }
void QuickstartComponent::setPluginsScannedProvider(StatusProvider p)      { content->isPluginsScanned = std::move(p); }
void QuickstartComponent::setInstrumentTrackProvider(StatusProvider p)     { content->hasInstrumentTrack = std::move(p); }
void QuickstartComponent::setExistingTrackOpenedProvider(StatusProvider p) { content->hasOpenedExistingTrack = std::move(p); }
