/*
  ==============================================================================

    PluginManagerDialog.cpp

  ==============================================================================
*/

#include "PluginManagerDialog.h"

namespace
{
    constexpr int kHeaderHeight   = 96;
    constexpr int kFooterHeight   = 60;
    constexpr int kPaddingX       = 18;
    constexpr int kPaddingY       = 16;
    constexpr int kCornerRadius   = 12;

    const juce::Colour kBgColour          { 0xff1b1b25 };
    const juce::Colour kSurfaceColour     { 0xff2a2a3a };
    const juce::Colour kSurfaceLight      { 0xff32324a };
    const juce::Colour kBorderColour      { 0x33ffffff };
    const juce::Colour kAccentColour      { 0xff6366f1 }; // Indigo
    const juce::Colour kAccentSecondary   { 0xff8b5cf6 }; // Violet
    const juce::Colour kTextPrimary       { 0xfff1f5f9 };
    const juce::Colour kTextSecondary     { 0xffa0a9c0 };
}

PluginManagerDialog::PluginManagerDialog(juce::AudioPluginFormatManager& apfm,
                                         juce::KnownPluginList& list,
                                         juce::PropertiesFile* props,
                                         const juce::File& deadMansPedalFile)
    : pluginList(list)
{
    // deadMansPedalFile (NICHT leer lassen!): JUCE traegt hier vor dem Laden
    // jedes Plugin ein und entfernt es nach erfolgreichem Scan wieder. Crasht
    // ein Plugin beim Scan (z.B. NI Transient Master in seiner Telemetrie-
    // Init), bleibt der Eintrag stehen und wird beim naechsten Scan
    // automatisch geblacklistet — sonst crasht der Scan bei JEDEM Versuch.
    listComponent = std::make_unique<juce::PluginListComponent>(apfm, list, deadMansPedalFile, props);
    addAndMakeVisible(listComponent.get());

    // Push our palette into the JUCE list so the inner table/buttons match
    // the dark surface around them instead of clashing.
    listComponent->setColour(juce::ListBox::backgroundColourId,         kSurfaceColour);
    listComponent->setColour(juce::ListBox::outlineColourId,            kBorderColour);
    listComponent->setColour(juce::ListBox::textColourId,               kTextPrimary);
    listComponent->setColour(juce::TableListBox::backgroundColourId,    kSurfaceColour);
    listComponent->setColour(juce::TableListBox::outlineColourId,       kBorderColour);
    listComponent->setColour(juce::TableHeaderComponent::backgroundColourId, kSurfaceLight);
    listComponent->setColour(juce::TableHeaderComponent::textColourId,  kTextPrimary);
    listComponent->setColour(juce::TableHeaderComponent::outlineColourId, kBorderColour);
    listComponent->setColour(juce::TextButton::buttonColourId,          kSurfaceLight);
    listComponent->setColour(juce::TextButton::textColourOffId,         kTextPrimary);

    closeButton = std::make_unique<juce::TextButton>("Done");
    closeButton->setColour(juce::TextButton::buttonColourId,   kAccentColour);
    closeButton->setColour(juce::TextButton::buttonOnColourId, kAccentSecondary);
    closeButton->setColour(juce::TextButton::textColourOffId,  juce::Colours::white);
    closeButton->setColour(juce::TextButton::textColourOnId,   juce::Colours::white);
    closeButton->addListener(this);
    addAndMakeVisible(closeButton.get());

    pluginList.addChangeListener(this);

    setOpaque(true);
    setSize(720, 620);
}

PluginManagerDialog::~PluginManagerDialog()
{
    pluginList.removeChangeListener(this);
    if (closeButton != nullptr)
        closeButton->removeListener(this);
}

void PluginManagerDialog::resized()
{
    auto area = getLocalBounds();

    headerBounds = area.removeFromTop(kHeaderHeight).reduced(kPaddingX, 0)
                       .withTrimmedTop(kPaddingY);

    footerBounds = area.removeFromBottom(kFooterHeight).reduced(kPaddingX, 0)
                       .withTrimmedBottom(kPaddingY);

    listBounds = area.reduced(kPaddingX, 8);

    if (listComponent != nullptr)
        listComponent->setBounds(listBounds.reduced(8));

    if (closeButton != nullptr)
    {
        const int w = 140;
        const int h = 38;
        closeButton->setBounds(footerBounds.getRight() - w,
                               footerBounds.getCentreY() - h / 2,
                               w, h);
    }
}

void PluginManagerDialog::paint(juce::Graphics& g)
{
    g.fillAll(kBgColour);

    // Subtle radial glow behind the header for depth.
    {
        juce::ColourGradient bgGlow(
            kAccentColour.withAlpha(0.18f),
            (float)getWidth() * 0.5f, -80.0f,
            kBgColour.withAlpha(0.0f),
            (float)getWidth() * 0.5f, (float)getHeight() * 0.6f,
            true);
        g.setGradientFill(bgGlow);
        g.fillAll();
    }

    paintHeader(g, headerBounds);

    // Surface panel behind the list.
    {
        auto panel = listBounds.toFloat();
        juce::DropShadow shadow(juce::Colours::black.withAlpha(0.35f), 12, { 0, 4 });
        juce::Path p;
        p.addRoundedRectangle(panel, (float)kCornerRadius);
        shadow.drawForPath(g, p);

        g.setColour(kSurfaceColour);
        g.fillRoundedRectangle(panel, (float)kCornerRadius);
        g.setColour(kBorderColour);
        g.drawRoundedRectangle(panel, (float)kCornerRadius, 1.0f);
    }
}

void PluginManagerDialog::paintHeader(juce::Graphics& g, juce::Rectangle<int> area)
{
    if (area.isEmpty())
        return;

    juce::Path hero;
    hero.addRoundedRectangle(area.toFloat(), (float)kCornerRadius * 1.2f);

    juce::ColourGradient grad(
        kAccentColour, (float)area.getX(), (float)area.getY(),
        kAccentSecondary, (float)area.getRight(), (float)area.getBottom(),
        false);
    grad.addColour(0.55, juce::Colour(0xff5b21b6));
    g.setGradientFill(grad);
    g.fillPath(hero);

    g.setColour(juce::Colours::white.withAlpha(0.08f));
    g.drawLine((float)area.getX() + 16, (float)area.getY() + 1,
               (float)area.getRight() - 16, (float)area.getY() + 1, 1.0f);

    auto inner = area.reduced(22, 14);

    g.setColour(juce::Colours::white.withAlpha(0.85f));
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.drawText("PLUGIN MANAGER", inner.removeFromTop(16), juce::Justification::topLeft, false);

    inner.removeFromTop(2);

    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(24.0f, juce::Font::bold));
    g.drawText("Manage your VST/AU plugins",
               inner.removeFromTop(30), juce::Justification::topLeft, false);

    // Plugin count badge on the right.
    {
        const int count = pluginList.getNumTypes();
        const juce::String label = juce::String(count) + (count == 1 ? " plugin" : " plugins");
        const int badgeW = 130;
        const int badgeH = 32;
        juce::Rectangle<int> badge(area.getRight() - badgeW - 22,
                                   area.getCentreY() - badgeH / 2,
                                   badgeW, badgeH);
        g.setColour(juce::Colours::black.withAlpha(0.30f));
        g.fillRoundedRectangle(badge.toFloat(), badgeH / 2.0f);
        g.setColour(juce::Colours::white.withAlpha(0.25f));
        g.drawRoundedRectangle(badge.toFloat(), badgeH / 2.0f, 1.0f);

        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(13.0f, juce::Font::bold));
        g.drawText(label, badge, juce::Justification::centred, false);
    }
}

void PluginManagerDialog::changeListenerCallback(juce::ChangeBroadcaster*)
{
    // Plugin count likely changed — repaint the badge.
    repaint(headerBounds);
}

void PluginManagerDialog::buttonClicked(juce::Button* b)
{
    if (b == closeButton.get())
    {
        if (auto* dw = findParentComponentOfClass<juce::DialogWindow>())
            dw->exitModalState(0);
    }
}

juce::DialogWindow* PluginManagerDialog::show(juce::AudioPluginFormatManager& apfm,
                                              juce::KnownPluginList& list,
                                              juce::PropertiesFile* props,
                                              juce::LookAndFeel* lf,
                                              const juce::File& deadMansPedalFile)
{
    auto* dialog = new PluginManagerDialog(apfm, list, props, deadMansPedalFile);
    if (lf != nullptr)
        dialog->setLookAndFeel(lf);

    juce::DialogWindow::LaunchOptions opts;
    opts.dialogTitle                  = "Plugin Manager";
    opts.escapeKeyTriggersCloseButton = true;
    opts.resizable                    = true;
    opts.useNativeTitleBar            = false;
    opts.useBottomRightCornerResizer  = true;
    opts.componentToCentreAround      = nullptr;
    opts.dialogBackgroundColour       = kBgColour;
    opts.content.setOwned(dialog);
    opts.content->setSize(720, 620);

    return opts.launchAsync();
}
