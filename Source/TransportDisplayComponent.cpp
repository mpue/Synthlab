#include "TransportDisplayComponent.h"

using namespace juce;

TransportDisplayComponent::TransportDisplayComponent()
{
    startTimerHz(30); // für BPM-Animation
    setOpaque(true);
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

void TransportDisplayComponent::setLayoutMode(LayoutMode mode)
{
    layoutMode = mode;
    repaint();
}

void TransportDisplayComponent::paint(Graphics& g)
{
    g.fillAll(Colour(25, 25, 30));

    g.setColour(Colours::darkgrey.withAlpha(0.8f));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(2), 8.0f, 2.0f);

    if (layoutMode == LayoutMode::Horizontal)
        drawHorizontalLayout(g);
    else
        drawCompactLayout(g);

    // Milchglas-Schicht oben drüber
    auto boundsF = getLocalBounds().toFloat();
    Colour glassColor = Colour::fromRGBA(180, 180, 200, 60); // milchiges Hellgrau-Blau
    g.setColour(glassColor);
    g.fillRoundedRectangle(boundsF.reduced(2), 12.0f);

    // Leichter Lichtreflex oben
    auto lightStripe = boundsF.withHeight(boundsF.getHeight() * 0.2f);
    g.setGradientFill(ColourGradient::vertical(Colours::white.withAlpha(0.15f), lightStripe.getY(),
        Colours::transparentWhite, lightStripe.getBottom()));
    g.fillRect(lightStripe);

}

void TransportDisplayComponent::drawHorizontalLayout(Graphics& g)
{
    auto bounds = getLocalBounds().reduced(12);
    g.setFont(Font(20.0f, Font::bold));
    g.setColour(Colours::white);

    String timeStr = getTimeAsString(currentSamplePos, currentSampleRate);
    String sampleStr = String((int64_t)currentSamplePos) + " samples";
    String sampleRateStr = String((int)currentSampleRate) + " Hz";
    String tempoStr = String((int)currentBpm) + " BPM";
    String timeSigStr = String(timeSigNumerator) + "/" + String(timeSigDenominator);

    g.drawFittedText(timeStr, bounds.removeFromLeft(160), Justification::left, 1);
    g.drawFittedText(sampleStr, bounds.removeFromLeft(180), Justification::left, 1);
    g.drawFittedText(sampleRateStr, bounds.removeFromLeft(120), Justification::left, 1);
    g.setColour(Colours::orange.withBrightness(0.8f + 0.2f * std::sin(bpmPulse * 0.15f)));
    g.drawFittedText(tempoStr, bounds.removeFromLeft(100), Justification::left, 1);
    g.setColour(Colours::white);
    g.drawFittedText("Sig: " + timeSigStr, bounds.removeFromLeft(80), Justification::left, 1);

    metronomeArea = bounds.removeFromLeft(30);
    drawMetronomeButton(g, metronomeArea);
}

void TransportDisplayComponent::drawCompactLayout(Graphics& g)
{
    auto bounds = getLocalBounds().reduced(8);
    g.setFont(Font(16.0f, Font::plain));
    g.setColour(Colours::white);

    g.drawFittedText(getTimeAsString(currentSamplePos, currentSampleRate), bounds.removeFromTop(20), Justification::centred, 1);
    g.drawFittedText(String((int64_t)currentSamplePos) + " samples", bounds.removeFromTop(20), Justification::centred, 1);
    g.drawFittedText(String((int)currentSampleRate) + " Hz", bounds.removeFromTop(20), Justification::centred, 1);

    g.setColour(Colours::orange.withBrightness(0.8f + 0.2f * std::sin(bpmPulse * 0.15f)));
    g.drawFittedText(String((int)currentBpm) + " BPM", bounds.removeFromTop(20), Justification::centred, 1);
    g.setColour(Colours::white);
    g.drawFittedText("Sig: " + String(timeSigNumerator) + "/" + String(timeSigDenominator), bounds.removeFromTop(20), Justification::centred, 1);

    metronomeArea = bounds.removeFromTop(30);
    drawMetronomeButton(g, metronomeArea);
}

void TransportDisplayComponent::drawMetronomeButton(Graphics& g, const Rectangle<int>& area)
{
    Colour fill = metronomeActive ? Colours::lime : Colours::darkred;
    g.setColour(fill);
    g.fillEllipse(area.toFloat().reduced(3.0f));

    g.setColour(Colours::white);
    g.drawEllipse(area.toFloat().reduced(3.0f), 2.0f);
}

void TransportDisplayComponent::resized() {}

void TransportDisplayComponent::mouseUp(const MouseEvent& e)
{
    if (metronomeArea.contains(e.getPosition()))
    {
        metronomeActive = !metronomeActive;
        if (onToggleMetronome)
            onToggleMetronome();
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
    // Mit Millisekunden
    double seconds = samples / rate;
    int h = static_cast<int>(seconds / 3600);
    int m = static_cast<int>(seconds / 60) % 60;
    int s = static_cast<int>(seconds) % 60;
    int ms = static_cast<int>((seconds - std::floor(seconds)) * 1000);
    return String::formatted("%02d:%02d:%02d.%03d", h, m, s, ms);
}
