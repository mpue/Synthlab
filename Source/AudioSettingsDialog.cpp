/*
  ==============================================================================

    AudioSettingsDialog.cpp

  ==============================================================================
*/

#include "AudioSettingsDialog.h"

namespace
{
    constexpr int kHeaderHeight = 96;
    constexpr int kFooterHeight = 60;
    constexpr int kPaddingX     = 18;
    constexpr int kPaddingY     = 16;
    constexpr int kCornerRadius = 12;

    const juce::Colour kBgColour          { 0xff1b1b25 };
    const juce::Colour kSurfaceColour     { 0xff2a2a3a };
    const juce::Colour kSurfaceLight      { 0xff32324a };
    const juce::Colour kBorderColour      { 0x33ffffff };
    const juce::Colour kAccentColour      { 0xff6366f1 }; // Indigo
    const juce::Colour kAccentSecondary   { 0xff8b5cf6 }; // Violet
    const juce::Colour kAccentSuccess     { 0xff10b981 }; // Emerald — "device live" pulse
    const juce::Colour kTextPrimary       { 0xfff1f5f9 };
}

AudioSettingsDialog::AudioSettingsDialog(juce::AudioDeviceManager& dm)
    : deviceManager(dm)
{
    selector = std::make_unique<juce::AudioDeviceSelectorComponent>(
        deviceManager,
        2, 16,   // input channels min/max
        2, 16,   // output channels min/max
        true,    // show MIDI inputs
        true,    // show MIDI output selector
        true,    // show channels-as-stereo-pairs
        false);  // hide advanced

    // Apply Synthlab palette to the inner JUCE controls so they don't clash
    // with the dark surface around them.
    selector->setColour(juce::ListBox::backgroundColourId,    kSurfaceColour);
    selector->setColour(juce::ListBox::outlineColourId,       kBorderColour);
    selector->setColour(juce::ListBox::textColourId,          kTextPrimary);
    selector->setColour(juce::ComboBox::backgroundColourId,   kSurfaceLight);
    selector->setColour(juce::ComboBox::textColourId,         kTextPrimary);
    selector->setColour(juce::ComboBox::outlineColourId,      kBorderColour);
    selector->setColour(juce::ComboBox::buttonColourId,       kSurfaceLight);
    selector->setColour(juce::ComboBox::arrowColourId,        kTextPrimary);
    selector->setColour(juce::TextButton::buttonColourId,     kSurfaceLight);
    selector->setColour(juce::TextButton::textColourOffId,    kTextPrimary);
    selector->setColour(juce::Label::textColourId,            kTextPrimary);
    selector->setColour(juce::ToggleButton::textColourId,     kTextPrimary);
    selector->setColour(juce::ToggleButton::tickColourId,     kAccentColour);
    selector->setColour(juce::ToggleButton::tickDisabledColourId, kBorderColour);

    addAndMakeVisible(selector.get());

    closeButton = std::make_unique<juce::TextButton>("Done");
    closeButton->setColour(juce::TextButton::buttonColourId,   kAccentColour);
    closeButton->setColour(juce::TextButton::buttonOnColourId, kAccentSecondary);
    closeButton->setColour(juce::TextButton::textColourOffId,  juce::Colours::white);
    closeButton->setColour(juce::TextButton::textColourOnId,   juce::Colours::white);
    closeButton->addListener(this);
    addAndMakeVisible(closeButton.get());

    deviceManager.addChangeListener(this);

    setOpaque(true);
    setSize(720, 620);
}

AudioSettingsDialog::~AudioSettingsDialog()
{
    deviceManager.removeChangeListener(this);
    if (closeButton != nullptr)
        closeButton->removeListener(this);
}

void AudioSettingsDialog::resized()
{
    auto area = getLocalBounds();

    headerBounds = area.removeFromTop(kHeaderHeight).reduced(kPaddingX, 0)
                       .withTrimmedTop(kPaddingY);

    footerBounds = area.removeFromBottom(kFooterHeight).reduced(kPaddingX, 0)
                       .withTrimmedBottom(kPaddingY);

    selectorBounds = area.reduced(kPaddingX, 8);

    if (selector != nullptr)
        selector->setBounds(selectorBounds.reduced(8));

    if (closeButton != nullptr)
    {
        const int w = 140;
        const int h = 38;
        closeButton->setBounds(footerBounds.getRight() - w,
                               footerBounds.getCentreY() - h / 2,
                               w, h);
    }
}

void AudioSettingsDialog::paint(juce::Graphics& g)
{
    g.fillAll(kBgColour);

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

    {
        auto panel = selectorBounds.toFloat();
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

void AudioSettingsDialog::paintHeader(juce::Graphics& g, juce::Rectangle<int> area)
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
    g.drawText("AUDIO SETTINGS", inner.removeFromTop(16), juce::Justification::topLeft, false);

    inner.removeFromTop(2);

    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(24.0f, juce::Font::bold));
    g.drawText("Configure your audio interface",
               inner.removeFromTop(30), juce::Justification::topLeft, false);

    // Device-status badge on the right — green dot + device name when live.
    {
        const juce::String label = currentDeviceSummary();
        juce::Font badgeFont(13.0f, juce::Font::bold);
        const int textWidth = badgeFont.getStringWidth(label);
        const int badgeW = juce::jmin(textWidth + 38, 320);
        const int badgeH = 32;
        juce::Rectangle<int> badge(area.getRight() - badgeW - 22,
                                   area.getCentreY() - badgeH / 2,
                                   badgeW, badgeH);
        g.setColour(juce::Colours::black.withAlpha(0.30f));
        g.fillRoundedRectangle(badge.toFloat(), badgeH / 2.0f);
        g.setColour(juce::Colours::white.withAlpha(0.25f));
        g.drawRoundedRectangle(badge.toFloat(), badgeH / 2.0f, 1.0f);

        const bool live = deviceManager.getCurrentAudioDevice() != nullptr
                       && deviceManager.getCurrentAudioDevice()->isOpen();

        const float dotR = 4.0f;
        const float dotX = (float)badge.getX() + 14.0f;
        const float dotY = (float)badge.getCentreY();
        g.setColour(live ? kAccentSuccess : juce::Colours::white.withAlpha(0.4f));
        g.fillEllipse(dotX - dotR, dotY - dotR, dotR * 2, dotR * 2);

        g.setColour(juce::Colours::white);
        g.setFont(badgeFont);
        g.drawText(label,
                   badge.withTrimmedLeft(24).withTrimmedRight(10),
                   juce::Justification::centredLeft, true);
    }
}

juce::String AudioSettingsDialog::currentDeviceSummary() const
{
    auto* dev = deviceManager.getCurrentAudioDevice();
    if (dev == nullptr || ! dev->isOpen())
        return "No device";

    const double sr = dev->getCurrentSampleRate();
    return dev->getName() + " · " + juce::String(sr / 1000.0, 1) + " kHz";
}

void AudioSettingsDialog::changeListenerCallback(juce::ChangeBroadcaster*)
{
    repaint(headerBounds);
}

void AudioSettingsDialog::buttonClicked(juce::Button* b)
{
    if (b == closeButton.get())
    {
        if (auto* dw = findParentComponentOfClass<juce::DialogWindow>())
            dw->exitModalState(0);
    }
}

juce::DialogWindow* AudioSettingsDialog::show(juce::AudioDeviceManager& dm,
                                              juce::LookAndFeel* lf)
{
    auto* dialog = new AudioSettingsDialog(dm);
    if (lf != nullptr)
        dialog->setLookAndFeel(lf);

    juce::DialogWindow::LaunchOptions opts;
    opts.dialogTitle                  = "Audio Settings";
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
