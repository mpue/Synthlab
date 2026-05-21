/*
  ==============================================================================

    AudioSettingsDialog.cpp

  ==============================================================================
*/

#include "AudioSettingsDialog.h"
#include "Settings.h"

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

    void applySynthlabSelectorPalette(juce::Component& c)
    {
        c.setColour(juce::ListBox::backgroundColourId,    kSurfaceColour);
        c.setColour(juce::ListBox::outlineColourId,       kBorderColour);
        c.setColour(juce::ListBox::textColourId,          kTextPrimary);
        c.setColour(juce::ComboBox::backgroundColourId,   kSurfaceLight);
        c.setColour(juce::ComboBox::textColourId,         kTextPrimary);
        c.setColour(juce::ComboBox::outlineColourId,      kBorderColour);
        c.setColour(juce::ComboBox::buttonColourId,       kSurfaceLight);
        c.setColour(juce::ComboBox::arrowColourId,        kTextPrimary);
        c.setColour(juce::TextButton::buttonColourId,     kSurfaceLight);
        c.setColour(juce::TextButton::textColourOffId,    kTextPrimary);
        c.setColour(juce::Label::textColourId,            kTextPrimary);
        c.setColour(juce::ToggleButton::textColourId,     kTextPrimary);
        c.setColour(juce::ToggleButton::tickColourId,     kAccentColour);
        c.setColour(juce::ToggleButton::tickDisabledColourId, kBorderColour);
    }
}

// =============================================================================
// General settings tab — currently exposes the output type used when creating
// new audio tracks (AudioOut vs. AuxOut). Backed by Settings::getInstance().
// =============================================================================
class GeneralSettingsPanel : public juce::Component,
                             private juce::ComboBox::Listener
{
public:
    GeneralSettingsPanel()
    {
        outputTypeLabel.setText("Output type when creating tracks:", juce::dontSendNotification);
        outputTypeLabel.setColour(juce::Label::textColourId, kTextPrimary);
        outputTypeLabel.setFont(juce::Font(15.0f, juce::Font::bold));
        addAndMakeVisible(outputTypeLabel);

        outputTypeBox.addItem("AudioOut", 1);
        outputTypeBox.addItem("AuxOut",   2);

        const auto current = Settings::getInstance()->getNewTrackOutputType();
        outputTypeBox.setSelectedId(
            current == Settings::NewTrackOutputType::AuxOut ? 2 : 1,
            juce::dontSendNotification);

        applySynthlabSelectorPalette(outputTypeBox);
        outputTypeBox.addListener(this);
        addAndMakeVisible(outputTypeBox);

        autoOpenPluginToggle.setButtonText("Open plugin window automatically");
        autoOpenPluginToggle.setColour(juce::ToggleButton::textColourId, kTextPrimary);
        autoOpenPluginToggle.setColour(juce::ToggleButton::tickColourId, kAccentColour);
        autoOpenPluginToggle.setColour(juce::ToggleButton::tickDisabledColourId, kBorderColour);
        autoOpenPluginToggle.setToggleState(
            Settings::getInstance()->getOpenPluginWindowAutomatically(),
            juce::dontSendNotification);
        autoOpenPluginToggle.onClick = [this]
        {
            Settings::getInstance()->setOpenPluginWindowAutomatically(
                autoOpenPluginToggle.getToggleState());
        };
        addAndMakeVisible(autoOpenPluginToggle);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(20);

        auto row = area.removeFromTop(34);
        outputTypeLabel.setBounds(row.removeFromLeft(260));
        outputTypeBox.setBounds(row.removeFromLeft(180));

        area.removeFromTop(10);
        auto row2 = area.removeFromTop(28);
        autoOpenPluginToggle.setBounds(row2.removeFromLeft(440));
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(kSurfaceColour);
    }

private:
    void comboBoxChanged(juce::ComboBox* box) override
    {
        if (box == &outputTypeBox)
        {
            Settings::getInstance()->setNewTrackOutputType(
                outputTypeBox.getSelectedId() == 2
                    ? Settings::NewTrackOutputType::AuxOut
                    : Settings::NewTrackOutputType::AudioOut);
        }
    }

    juce::Label        outputTypeLabel;
    juce::ComboBox     outputTypeBox;
    juce::ToggleButton autoOpenPluginToggle;
};

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

    applySynthlabSelectorPalette(*selector);

    generalPanel = std::make_unique<GeneralSettingsPanel>();

    tabs = std::make_unique<juce::TabbedComponent>(juce::TabbedButtonBar::TabsAtTop);
    tabs->setOutline(0);
    tabs->setIndent(0);
    tabs->setTabBarDepth(34);
    tabs->setColour(juce::TabbedComponent::backgroundColourId, kSurfaceColour);
    tabs->setColour(juce::TabbedComponent::outlineColourId,    kBorderColour);
    tabs->setColour(juce::TabbedButtonBar::tabOutlineColourId, kBorderColour);
    tabs->setColour(juce::TabbedButtonBar::tabTextColourId,    kTextPrimary.withAlpha(0.7f));
    tabs->setColour(juce::TabbedButtonBar::frontTextColourId,  kTextPrimary);
    tabs->setColour(juce::TabbedButtonBar::frontOutlineColourId, kAccentColour);

    // Tabs own the layout of these children but not their lifetime — we keep
    // the unique_ptrs as the source of truth (deleteWhenRemoved = false).
    tabs->addTab("Audio Device", kSurfaceColour, selector.get(),     false);
    tabs->addTab("General",      kSurfaceColour, generalPanel.get(), false);

    addAndMakeVisible(tabs.get());

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

    // TabbedComponent does not own the child components (we passed false to
    // addTab) — clear it before our unique_ptrs go out of scope, so it does
    // not retain dangling raw pointers during its own destruction.
    if (tabs != nullptr)
        tabs->clearTabs();
}

void AudioSettingsDialog::resized()
{
    auto area = getLocalBounds();

    headerBounds = area.removeFromTop(kHeaderHeight).reduced(kPaddingX, 0)
                       .withTrimmedTop(kPaddingY);

    footerBounds = area.removeFromBottom(kFooterHeight).reduced(kPaddingX, 0)
                       .withTrimmedBottom(kPaddingY);

    selectorBounds = area.reduced(kPaddingX, 8);

    if (tabs != nullptr)
        tabs->setBounds(selectorBounds.reduced(8));

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
