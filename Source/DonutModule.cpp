/*
  ==============================================================================
    DonutModule.cpp
    Created: 7 Oct 2025
    Author:  mpue
  ==============================================================================
*/
#include "DonutModule.h"

DonutModule::DonutModule()
{
    setName("Donut");

    // Torus/Donut Parameter
    const float majorRadius = 1.0f;  // Äußerer Radius
    const float minorRadius = 0.4f;  // Röhrenradius
    const int majorSegments = 32;    // Segmente um den Ring
    const int minorSegments = 16;    // Segmente der Röhre

    // Berechne Anzahl der Vertices und Indices
    const int numVertices = (majorSegments + 1) * (minorSegments + 1);
    const int numIndices = majorSegments * minorSegments * 6;

    // Allokiere Arrays (6 floats pro Vertex: Position (3) + Farbe (3))
    vertices = std::make_unique<float[]>(numVertices * 6);
    indices = std::make_unique<unsigned int[]>(numIndices);

    int vertexIndex = 0;

    // Generiere Vertices
    for (int i = 0; i <= majorSegments; i++)
    {
        float u = (float)i / (float)majorSegments;
        float theta = u * juce::MathConstants<float>::twoPi;
        float cosTheta = std::cos(theta);
        float sinTheta = std::sin(theta);

        for (int j = 0; j <= minorSegments; j++)
        {
            float v = (float)j / (float)minorSegments;
            float phi = v * juce::MathConstants<float>::twoPi;
            float cosPhi = std::cos(phi);
            float sinPhi = std::sin(phi);

            // Position - Korrigierte Torus-Formel
            float x = (majorRadius + minorRadius * cosPhi) * cosTheta;
            float y = minorRadius * sinPhi;  // Y ist die vertikale Achse
            float z = (majorRadius + minorRadius * cosPhi) * sinTheta;  // Z bildet mit X die horizontale Ebene

            // Farbe basierend auf Position (u, v Parameter für Farbverlauf)
            float r = (u + 1.0f) * 0.5f;
            float g = (v + 1.0f) * 0.5f;
            float b = (std::sin(theta * 2.0f) + 1.0f) * 0.5f;

            // Vertex-Daten schreiben
            vertices[vertexIndex++] = x;  // Position X
            vertices[vertexIndex++] = y;  // Position Y
            vertices[vertexIndex++] = z;  // Position Z
            vertices[vertexIndex++] = r;  // Farbe R
            vertices[vertexIndex++] = g;  // Farbe G
            vertices[vertexIndex++] = b;  // Farbe B
        }
    }

    // Generiere Indices (Counter-Clockwise Wicklung)
    int indexOffset = 0;
    for (int i = 0; i < majorSegments; i++)
    {
        for (int j = 0; j < minorSegments; j++)
        {
            int first = i * (minorSegments + 1) + j;
            int second = first + minorSegments + 1;

            // Erstes Dreieck (Counter-Clockwise)
            indices[indexOffset++] = first;
            indices[indexOffset++] = second;
            indices[indexOffset++] = first + 1;

            // Zweites Dreieck (Counter-Clockwise)
            indices[indexOffset++] = second;
            indices[indexOffset++] = second + 1;
            indices[indexOffset++] = first + 1;
        }
    }

    // Erstelle Geometry-Objekt
    donut = std::make_unique<Geometry>(vertices.get(), indices.get(), numIndices, numVertices);
}

DonutModule::~DonutModule()
{
    // Automatisches Cleanup durch Smart Pointers
}

void DonutModule::paint(juce::Graphics& g)
{
    Module::paint(g);

    // Paint a stylized donut shape
    int centerX = getWidth() / 2;
    int centerY = (getHeight() - 40) / 2 + 20;
    int outerRadius = std::min(getWidth(), getHeight() - 80) / 2 - 10;
    int innerRadius = outerRadius / 2;

    // Äußerer Kreis
    g.setColour(juce::Colours::orange);
    g.fillEllipse(centerX - outerRadius, centerY - outerRadius,
        outerRadius * 2, outerRadius * 2);

    // Inneres Loch
    g.setColour(juce::Colour(0xff2a2a2a));
    g.fillEllipse(centerX - innerRadius, centerY - innerRadius,
        innerRadius * 2, innerRadius * 2);
}

void DonutModule::process()
{
    pins.at(0)->setGeometry(donut.get());
}

void DonutModule::configurePins()
{
    Pin* p1 = new Pin(Pin::Type::GEOMETRY);
    p1->direction = Pin::Direction::OUT;
    p1->setName("Geom");
    addPin(p1->direction, p1);
}