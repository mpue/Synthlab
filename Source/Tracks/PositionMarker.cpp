#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

//==============================================================================

class PositionMarker : public Component
{
public:
    PositionMarker(int64 sampleLength)
        : sampleLength(sampleLength), samplePosition(0), zoom(40.0)
    {
        setInterceptsMouseClicks(false, false);
    }

    void paint(Graphics& g) override
    {
        g.setColour(Colours::green);
        g.fillRect(0, 0, 2, getHeight());
    }

    void setDrawingBounds(int x, int y, int width, int height)
    {
        this->x = x;
        this->y = y;
        this->width = width;
        this->height = height;
        updatePosition();
        setSize(2, height);
    }

    void setSampleLength(int64 samples)
    {
        this->sampleLength = samples;
        updatePosition();
        repaint();
    }

    void setSamplePosition(int64 samples)
    {
        samplePosition = std::max<int64>(0, samples);
        updatePosition();
        repaint();
    }

    int64 getSamplePosition() const
    {
        return samplePosition;
    }

    void setZoom(double newZoom)
    {
        zoom = newZoom;
        updatePosition();
    }

private:
    void updatePosition()
    {
        const int newX = static_cast<int>((double)samplePosition / zoom);
        setTopLeftPosition(newX, y);
    }

    int64 sampleLength = 0;
    int64 samplePosition = 0;

    double zoom = 40.0;

    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};
