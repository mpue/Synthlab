/*
  ==============================================================================

    SevenSegmentModule.cpp
    Created: 15 Jun 2026
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "SevenSegmentModule.h"
#include "../JuceLibraryCode/JuceHeader.h"

using juce::String;

namespace {
    // Segment masks for the digits 0-9 and a blank.
    // bit order: a,b,c,d,e,f,g  (bit0 = a ... bit6 = g)
    const int kDigitSegments[10] = {
        0b0111111, // 0 -> a b c d e f
        0b0000110, // 1 -> b c
        0b1011011, // 2 -> a b d e g
        0b1001111, // 3 -> a b c d g
        0b1100110, // 4 -> b c f g
        0b1101101, // 5 -> a c d f g
        0b1111101, // 6 -> a c d e f g
        0b0000111, // 7 -> a b c
        0b1111111, // 8 -> all
        0b1101111  // 9 -> a b c d f g
    };

    const int kSegMinus = 0b1000000; // only g
}

//==============================================================================
SevenSegmentModule::SevenSegmentModule() :
    valueInPin(nullptr)
{
    setSize(180,100);
    setName("7-Segment");
    editable = false;
    prefab = true;
    startTimer(50);
}

SevenSegmentModule::~SevenSegmentModule()
{
    stopTimer();
}

void SevenSegmentModule::configurePins() {
    valueInPin = new Pin(Pin::Type::VALUE);
    valueInPin->direction = Pin::Direction::IN;
    valueInPin->setName("V");
    addPin(Pin::Direction::IN,valueInPin);
}

void SevenSegmentModule::drawDigit(juce::Graphics &g, int segments,
                                   float x, float y, float w, float h)
{
    const juce::Colour on  = juce::Colour(0xffff3b30); // bright red
    const juce::Colour off = juce::Colour(0xff401010); // dim red

    const float t = juce::jmin(w, h) * 0.16f; // segment thickness
    const float p = t * 0.5f;                 // gap / inset
    const float midY = y + h * 0.5f;

    auto seg = [&](int bit, juce::Path path) {
        g.setColour((segments & bit) ? on : off);
        g.fillPath(path);
    };

    // Horizontal segment as a hexagon between (x1) and (x2) at vertical centre cy
    auto hSeg = [&](float x1, float x2, float cy) {
        juce::Path path;
        path.startNewSubPath(x1 + p,        cy);
        path.lineTo(x1 + p + t * 0.6f,      cy - t * 0.5f);
        path.lineTo(x2 - p - t * 0.6f,      cy - t * 0.5f);
        path.lineTo(x2 - p,                 cy);
        path.lineTo(x2 - p - t * 0.6f,      cy + t * 0.5f);
        path.lineTo(x1 + p + t * 0.6f,      cy + t * 0.5f);
        path.closeSubPath();
        return path;
    };

    // Vertical segment as a hexagon between (y1) and (y2) at horizontal centre cx
    auto vSeg = [&](float y1, float y2, float cx) {
        juce::Path path;
        path.startNewSubPath(cx,            y1 + p);
        path.lineTo(cx + t * 0.5f,          y1 + p + t * 0.6f);
        path.lineTo(cx + t * 0.5f,          y2 - p - t * 0.6f);
        path.lineTo(cx,                     y2 - p);
        path.lineTo(cx - t * 0.5f,          y2 - p - t * 0.6f);
        path.lineTo(cx - t * 0.5f,          y1 + p + t * 0.6f);
        path.closeSubPath();
        return path;
    };

    const float left  = x;
    const float right = x + w;
    const float top   = y;
    const float bot   = y + h;

    seg(0b0000001, hSeg(left, right, top));        // a  (top)
    seg(0b0000010, vSeg(top, midY, right));        // b  (top-right)
    seg(0b0000100, vSeg(midY, bot, right));        // c  (bottom-right)
    seg(0b0001000, hSeg(left, right, bot));        // d  (bottom)
    seg(0b0010000, vSeg(midY, bot, left));         // e  (bottom-left)
    seg(0b0100000, vSeg(top, midY, left));         // f  (top-left)
    seg(0b1000000, hSeg(left, right, midY));       // g  (middle)
}

void SevenSegmentModule::paint(juce::Graphics &g) {
    Module::paint(g);

    // Display area inside the module body
    const float pad     = 14.0f;
    const float topPad  = 26.0f;
    juce::Rectangle<float> area((float) pad, topPad,
                                getWidth() - 2 * pad,
                                getHeight() - topPad - pad);

    // Dark recessed background panel
    g.setColour(juce::Colour(0xff1a0707));
    g.fillRoundedRectangle(area, 4.0f);

    const float spacing  = area.getWidth() * 0.04f;
    const float cellW    = (area.getWidth() - spacing * (numDigits + 1)) / numDigits;
    const float cellH    = area.getHeight() * 0.8f;
    const float cellY    = area.getY() + (area.getHeight() - cellH) * 0.5f;

    // Build the digit string, right aligned, with optional leading '-'.
    bool negative = value < 0;
    long whole = (long) std::llround(std::fabs((double) value));

    // Clamp to what fits (all 9s) leaving room for a sign if needed.
    long maxVal = 1;
    for (int i = 0; i < numDigits; ++i) maxVal *= 10;
    if (whole >= maxVal) whole = maxVal - 1;

    juce::String digits(whole);
    while (digits.length() < numDigits)
        digits = " " + digits;

    for (int i = 0; i < numDigits; ++i) {
        float cellX = area.getX() + spacing + i * (cellW + spacing);
        juce::juce_wchar c = digits[i];

        if (c >= '0' && c <= '9') {
            // Show a leading minus in the cell just before the first real digit.
            bool firstDigit = (i > 0 && digits[i - 1] == ' ');
            if (firstDigit && negative)
                drawDigit(g, kSegMinus, cellX - cellW * 0.5f, cellY, cellW, cellH);

            drawDigit(g, kDigitSegments[c - '0'], cellX, cellY, cellW, cellH);
        }
        else {
            // blank cell: draw all segments off
            drawDigit(g, 0, cellX, cellY, cellW, cellH);
        }
    }
}

void SevenSegmentModule::process() {

    if (getPins().size() > 0 && getPins().at(0)->getConnections().size() > 0) {
        value = getPins().at(0)->getConnections().at(0)->getValue();
    }
}
