#include "TransportDisplayComponent.h"
#include "Project/Project.h"

using namespace juce;

TransportDisplayComponent::TransportDisplayComponent()
{
    startTimerHz(30); // für BPM-Animation
    setOpaque(true);

    tempoLabel = std::make_unique<Label>("TempoLabel", "BPM:");
    tempoLabel->setJustificationType(Justification::centredLeft);
    tempoLabel->setColour(Label::textColourId, lookAndFeel.textSecondary);
    tempoLabel->setFont(lookAndFeel.secondaryFont);
    addAndMakeVisible(*tempoLabel);

    tempoEditor = std::make_unique<TextEditor>("TempoEditor");
    tempoEditor->setInputRestrictions(4, "0123456789.");
    tempoEditor->setText(String(Project::getInstance()->getTempo()), dontSendNotification);
    tempoEditor->setFont(lookAndFeel.mainFont);
    tempoEditor->setColour(TextEditor::backgroundColourId, lookAndFeel.background.brighter(0.1f));
    tempoEditor->setColour(TextEditor::textColourId, lookAndFeel.text);
    tempoEditor->setColour(TextEditor::outlineColourId, lookAndFeel.border);
    tempoEditor->setColour(TextEditor::focusedOutlineColourId, lookAndFeel.accent);
    tempoEditor->onReturnKey = [this]() {
        double newTempo = tempoEditor->getText().getDoubleValue();
        if (newTempo >= 40.0 && newTempo <= 300.0) {
            Project::getInstance()->setTempo(newTempo);
        }
        else {
            tempoEditor->setText(String(Project::getInstance()->getTempo()), dontSendNotification);
        }
		sendChangeMessage();
    };
    addAndMakeVisible(*tempoEditor);
}

void TransportDisplayComponent::setTransportState(double samplePos, double sampleRate, double bpm, int numerator, int denominator)
{
    currentSamplePos = samplePos;
    currentSampleRate = sampleRate;
    currentBpm = bpm;
    timeSigNumerator = numerator;
    timeSigDenominator = denominator;
}

void TransportDisplayComponent::setMetronomeState(bool active)
{
    metronomeActive = active;
    repaint();
}

void TransportDisplayComponent::setLoopState(bool active)
{
    loopActive = active;
    repaint();
}

void TransportDisplayComponent::setLayoutMode(LayoutMode mode)
{
    layoutMode = mode;
    repaint();
}

void TransportDisplayComponent::paint(Graphics& g)
{
    auto boundsF = getLocalBounds().toFloat();

    // Gradient Background
    drawGradientBackground(g, boundsF);

    // Border
    g.setColour(lookAndFeel.border);
    g.drawRoundedRectangle(boundsF.reduced(1), lookAndFeel.cornerRadius, lookAndFeel.borderThickness);

    // Content
    if (layoutMode == LayoutMode::Horizontal)
        drawHorizontalLayout(g);
    else
        drawCompactLayout(g);

    // Glass Effect
    drawGlassEffect(g, boundsF);
}

void TransportDisplayComponent::drawGradientBackground(Graphics& g, const Rectangle<float>& bounds)
{
    ColourGradient gradient = ColourGradient::vertical(
        lookAndFeel.backgroundGradientStart, bounds.getY(),
        lookAndFeel.backgroundGradientEnd, bounds.getBottom()
    );
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(bounds, lookAndFeel.cornerRadius);
}

void TransportDisplayComponent::drawGlassEffect(Graphics& g, const Rectangle<float>& bounds)
{
    // Subtiler Glas-Effekt
    g.setColour(lookAndFeel.glassOverlay);
    g.fillRoundedRectangle(bounds.reduced(2), lookAndFeel.cornerRadius - 2);

    // Highlight oben
    auto highlight = bounds.withHeight(bounds.getHeight() * 0.25f).reduced(3);
    g.setGradientFill(ColourGradient::vertical(
        Colours::white.withAlpha(0.08f), highlight.getY(),
        Colours::transparentWhite, highlight.getBottom()
    ));
    g.fillRoundedRectangle(highlight, lookAndFeel.cornerRadius - 3);
}

void TransportDisplayComponent::drawHorizontalLayout(Graphics& g)
{
    auto bounds = getLocalBounds().reduced(16);

    // Zeit
    g.setFont(lookAndFeel.mainFont);
    g.setColour(lookAndFeel.text);
    String timeStr = getTimeAsString(currentSamplePos, currentSampleRate);
    g.drawFittedText(timeStr, bounds.removeFromLeft(170), Justification::left, 1);

    // Bar.Beat.Tick
    String barBeatStr = getBarBeatString(currentSamplePos, currentSampleRate, currentBpm, timeSigNumerator, timeSigDenominator);
    g.setColour(lookAndFeel.accent);
    g.drawFittedText(barBeatStr, bounds.removeFromLeft(120), Justification::left, 1);

    // Sample Rate
    g.setFont(lookAndFeel.secondaryFont);
    g.setColour(lookAndFeel.textSecondary);
    String sampleRateStr = String((int)currentSampleRate) + " Hz";
    g.drawFittedText(sampleRateStr, bounds.removeFromLeft(80), Justification::left, 1);

    // Time Signature
    String timeSigStr = String(timeSigNumerator) + "/" + String(timeSigDenominator);
    g.drawFittedText(timeSigStr, bounds.removeFromLeft(60), Justification::left, 1);

    // BPM Puls-Anzeige
    if (bpmPulse * 2 % 60 < 10) // Kurzes Aufleuchten
    {
        g.setColour(lookAndFeel.tempoPulse.withAlpha(0.7f));
        auto pulseArea = bounds.removeFromLeft(20);
        g.fillEllipse(pulseArea.toFloat().reduced(6));
    }
    else {
        g.setColour(lookAndFeel.background);
        auto pulseArea = bounds.removeFromLeft(20);
        g.fillEllipse(pulseArea.toFloat().reduced(6));
    }

    bounds.removeFromLeft(20); // Platz für Puls

    // Buttons
    metronomeArea = bounds.removeFromLeft(40);
    drawMetronomeButton(g, metronomeArea);

    bounds.removeFromLeft(8); // Spacing

    loopArea = bounds.removeFromLeft(40);
    drawLoopButton(g, loopArea);
}

void TransportDisplayComponent::drawCompactLayout(Graphics& g)
{
    auto bounds = getLocalBounds().reduced(12);

    // Zeit
    g.setFont(lookAndFeel.mainFont);
    g.setColour(lookAndFeel.text);
    g.drawFittedText(getTimeAsString(currentSamplePos, currentSampleRate),
        bounds.removeFromTop(24), Justification::centred, 1);

    // Bar.Beat.Tick
    g.setColour(lookAndFeel.accent);
    String barBeatStr = getBarBeatString(currentSamplePos, currentSampleRate, currentBpm, timeSigNumerator, timeSigDenominator);
    g.drawFittedText(barBeatStr, bounds.removeFromTop(22), Justification::centred, 1);

    // Sample Rate & Time Sig
    g.setFont(lookAndFeel.secondaryFont);
    g.setColour(lookAndFeel.textSecondary);
    String infoStr = String((int)currentSampleRate) + " Hz | " + String(timeSigNumerator) + "/" + String(timeSigDenominator);
    g.drawFittedText(infoStr, bounds.removeFromTop(20), Justification::centred, 1);

    // Buttons nebeneinander
    auto buttonArea = bounds.removeFromTop(36);
    metronomeArea = buttonArea.removeFromLeft(buttonArea.getWidth() / 2).reduced(4);
    loopArea = buttonArea.reduced(4);

    drawMetronomeButton(g, metronomeArea);
    drawLoopButton(g, loopArea);
}

void TransportDisplayComponent::drawMetronomeButton(Graphics& g, const Rectangle<int>& area)
{
    auto bounds = area.toFloat().reduced(2);

    // Background
    Colour fillColour = metronomeActive ? lookAndFeel.metronomeOn : lookAndFeel.metronomeOff;
    g.setColour(fillColour);
    g.fillRoundedRectangle(bounds, 6.0f);

    // Border
    g.setColour(fillColour.brighter(0.3f));
    g.drawRoundedRectangle(bounds, 6.0f, 1.5f);

    // Icon (Metronom-Symbol)
    g.setColour(Colours::white);
    auto iconBounds = bounds.reduced(8);
    g.fillEllipse(iconBounds.withSizeKeepingCentre(4, 4)); // Punkt oben

    // Pendel-Linie
    g.drawLine(iconBounds.getCentreX(), iconBounds.getCentreY() - 2,
        iconBounds.getCentreX() + (metronomeActive ? 6 : -6), iconBounds.getBottom() - 2, 2.0f);
}

void TransportDisplayComponent::drawLoopButton(Graphics& g, const Rectangle<int>& area)
{
    auto bounds = area.toFloat().reduced(2);

    // Background
    Colour fillColour = loopActive ? lookAndFeel.loopOn : lookAndFeel.loopOff;
    g.setColour(fillColour);
    g.fillRoundedRectangle(bounds, 6.0f);

    // Border
    g.setColour(fillColour.brighter(0.3f));
    g.drawRoundedRectangle(bounds, 6.0f, 1.5f);

    // Loop-Symbol (Pfeil-Kreis)
    g.setColour(Colours::white);
    auto iconBounds = bounds.reduced(8);
    auto center = iconBounds.getCentre();
    float radius = iconBounds.getWidth() / 3.0f;

    // Kreis
    g.drawEllipse(center.x - radius, center.y - radius, radius * 2, radius * 2, 2.0f);

    // Pfeil
    Path arrow;
    arrow.startNewSubPath(center.x + radius - 2, center.y - radius + 2);
    arrow.lineTo(center.x + radius + 2, center.y - radius - 2);
    arrow.lineTo(center.x + radius + 2, center.y - radius + 6);
    arrow.closeSubPath();
    g.fillPath(arrow);
}

void TransportDisplayComponent::resized()
{
    if (layoutMode == LayoutMode::Horizontal)
    {
        int tempoX = getWidth() - 140;
        tempoLabel->setBounds(tempoX, 16, 35, 20);
        tempoEditor->setBounds(tempoX + 35, 14, 60, 24);
    }
    else
    {
        int tempoX = (getWidth() - 95) / 2;
        tempoLabel->setBounds(tempoX, getHeight() - 30, 35, 20);
        tempoEditor->setBounds(tempoX + 35, getHeight() - 32, 60, 24);
    }
}

void TransportDisplayComponent::mouseUp(const MouseEvent& e)
{
    if (metronomeArea.contains(e.getPosition()))
    {
        metronomeActive = !metronomeActive;
        if (onToggleMetronome)
            onToggleMetronome();
        repaint();
    }
    else if (loopArea.contains(e.getPosition()))
    {
        loopActive = !loopActive;
        if (onToggleLoop)
            onToggleLoop();
        repaint();
    }
}

void TransportDisplayComponent::timerCallback()
{
    bpmPulse++;
    repaint();
}

String TransportDisplayComponent::getTimeAsString(double samples, double rate) const
{
    double seconds = samples / rate;
    int h = static_cast<int>(seconds / 3600);
    int m = static_cast<int>(seconds / 60) % 60;
    int s = static_cast<int>(seconds) % 60;
    int ms = static_cast<int>((seconds - std::floor(seconds)) * 1000);
    return String::formatted("%02d:%02d:%02d.%03d", h, m, s, ms);
}

String TransportDisplayComponent::getBarBeatString(double samples, double rate, double bpm, int numerator, int denominator) const
{
    if (rate <= 0.0 || bpm <= 0.0 || denominator <= 0)
        return "0.0.0";

    double seconds = samples / rate;
    double beatsPerSecond = bpm / 60.0;
    double totalBeats = seconds * beatsPerSecond;

    int beatsPerBar = numerator;
    int totalSixteenths = static_cast<int>(totalBeats * 4.0);

    int bar = totalSixteenths / (beatsPerBar * 4) + 1;
    int beat = (totalSixteenths / 4) % beatsPerBar + 1;
    int sixteenth = totalSixteenths % 4 + 1;

    return String(bar) + "." + String(beat) + "." + String(sixteenth);
}