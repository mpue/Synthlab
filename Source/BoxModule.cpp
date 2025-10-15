/*
  ==============================================================================

    BoxModule.cpp
    Created: 7 Oct 2025 2:41:48pm
    Author:  mpue

  ==============================================================================
*/

#include "BoxModule.h"

BoxModule::BoxModule()
{
	setName("Box");
	prefab = true;
	editable = false;
    vertices = new float[48] {
        // Position          // Farbe
        // Vordere Seite (z = 0.5)
        -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  // 0: Vorne-Unten-Links (Rot)
         0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f,  // 1: Vorne-Unten-Rechts (Grün)
         0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  // 2: Vorne-Oben-Rechts (Blau)
        -0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f,  // 3: Vorne-Oben-Links (Gelb)

        // Hintere Seite (z = -0.5)
        -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f,  // 4: Hinten-Unten-Links (Magenta)
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 1.0f,  // 5: Hinten-Unten-Rechts (Cyan)
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 1.0f,  // 6: Hinten-Oben-Rechts (Weiß)
        -0.5f,  0.5f, -0.5f,  0.5f, 0.5f, 0.5f   // 7: Hinten-Oben-Links (Grau)
    };

    // Korrigierte Indizes mit konsistenter Counter-Clockwise Wicklung
    // (von außen betrachtet)
    indices= new unsigned int[36] {
        // Vordere Seite (z = 0.5)
        0, 1, 2,  2, 3, 0,

        // Rechte Seite (x = 0.5)
        1, 5, 6,  6, 2, 1,

        // Hintere Seite (z = -0.5)
        5, 4, 7,  7, 6, 5,

        // Linke Seite (x = -0.5)
        4, 0, 3,  3, 7, 4,

        // Obere Seite (y = 0.5)
        3, 2, 6,  6, 7, 3,

        // Untere Seite (y = -0.5)
        4, 5, 1,  1, 0, 4
    };

	box = new Geometry(vertices, indices, 48,36);
}

BoxModule::~BoxModule()
{
}

void BoxModule::paint(juce::Graphics& g)
{
    Module::paint(g);
}

void BoxModule::process()
{
    pins.at(0)->setGeometry(box);
}

void BoxModule::configurePins()
{
    Pin* p1 = new Pin(Pin::Type::GEOMETRY);
    p1->direction = Pin::Direction::OUT;
    p1->setName("Geom");

	addPin(p1->direction, p1);  
}
