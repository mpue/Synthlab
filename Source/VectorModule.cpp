/*
  ==============================================================================

    VectorModule.cpp
    Created: 6 Oct 2025 7:37:45pm
    Author:  mpue

  ==============================================================================
*/

#include "VectorModule.h"

VectorModule::VectorModule()
{
    setSize(120,140);
    nameLabel->setJustificationType (Justification::left);
    nameLabel->setTopLeftPosition(18,2);
    setName("Vector");
    editable = false;
    prefab = true;
    setInterceptsMouseClicks(false,true);
}

VectorModule::~VectorModule()
{
}

void VectorModule::paint(juce::Graphics& g)
{
	Module::paint(g);
}

void VectorModule::process()
{
	float x = 0;
	float y = 0;
	float z = 0;

    if (pins.at(0)->getConnections().size() == 1) {
        x = pins.at(0)->getConnections().at(0)->getValue();
    }
    if (pins.at(1)->getConnections().size() == 1) {
        y = pins.at(1)->getConnections().at(0)->getValue();
	}
    if (pins.at(2)->getConnections().size() == 1) {
        z = pins.at(2)->getConnections().at(0)->getValue();
    }
	vector = { x, y, z };

	pins.at(3)->setVector3D(vector);

}

void VectorModule::configurePins()
{
    Pin* p1 = new Pin(Pin::Type::VALUE);
    p1->direction = Pin::Direction::IN;
    p1->setName("X");

    Pin* p2 = new Pin(Pin::Type::VALUE);
    p2->direction = Pin::Direction::IN;
    p2->setName("Y");

    Pin* p3 = new Pin(Pin::Type::VALUE);
    p3->direction = Pin::Direction::IN;
    p3->setName("Z");

    Pin* p4 = new Pin(Pin::Type::VECTOR);
    p4->direction = Pin::Direction::OUT;        
    p4->setName("V");

    addPin(p1->direction, p1);
    addPin(p2->direction, p2);
    addPin(p3->direction, p3);
    addPin(p4->direction, p4);
}


