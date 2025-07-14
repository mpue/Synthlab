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
    
    inline int  timeToSamples(double sec, double sr) { return (int)(sec * sr + 0.5); }
    inline double samplesToTime(int smp, double sr) { return smp / sr; }

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
        if (samplePosition % 2 != 0)
            samplePosition -= 1;
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

    void setSampleRate(double rate) {
        sampleRate = rate;
        updatePosition();
    }

private:
    void updatePosition()
    {
        if (sampleLength > 0 && sampleRate > 0.0)
        {
            const double seconds = (double)samplePosition / sampleRate;
            const int newX = static_cast<int>(seconds * zoom);
            setTopLeftPosition(newX, y);
        }
    }


    int64 sampleLength = 0;
    int64 samplePosition = 0;

    double zoom = 40.0;

    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    double sampleRate = 44100.0;
};
