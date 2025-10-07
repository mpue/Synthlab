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
    // Torus/Donut Parameter
    const float majorRadius = 1.0f;  // Äußerer Radius
    const float minorRadius = 0.4f;  // Röhrenradius
    const int majorSegments = 32;    // Segmente um den Ring
    const int minorSegments = 16;    // Segmente der Röhre

    // Berechne Anzahl der Vertices und Indices
    const int numVertices = (majorSegments + 1) * (minorSegments + 1);
    const int numIndices = majorSegments * minorSegments * 6;

    // Allokiere Arrays (6 floats pro Vertex: Position (3) + Farbe (3))
    vertices = new float[numVertices * 6];
    indices = new unsigned int[numIndices];

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
    donut = new Geometry(vertices, indices, numIndices, numVertices);
}

DonutModule::~DonutModule()
{
}

void DonutModule::paint(juce::Graphics& g)
{
    Module::paint(g);
    g.setColour(juce::Colours::white);
    g.setFont(12);
    g.drawText("Donut", 10, getHeight() / 2 - 10, 100, 20, juce::Justification::centred);

    // paint a stilized donut shape
    g.setColour(juce::Colours::orange);
    g.fillEllipse(30, 30, getWidth() - 60, getHeight() - 80);
    g.setColour(juce::Colours::darkorange);
    g.fillEllipse(40, 40, getWidth() - 80, getHeight() - 100);
}

void DonutModule::process()
{
    pins.at(0)->setGeometry(donut);
}

void DonutModule::configurePins()
{
    Pin* p1 = new Pin(Pin::Type::GEOMETRY);
    p1->direction = Pin::Direction::OUT;
    p1->setName("Geom");
    addPin(p1->direction, p1);
}