/*
  ==============================================================================

    GeometryModule.cpp
    Created: 7 Oct 2025 11:09:10am
    Author:  mpue

  ==============================================================================
*/

#include "GeometryModule.h"

GeometryModule::GeometryModule()
{
    setSize(120, 140);
    nameLabel->setJustificationType(Justification::left);
    nameLabel->setTopLeftPosition(18, 2);
    setName("Geometry");
    editable = false;
    prefab = true;
    setInterceptsMouseClicks(false, true);
}

GeometryModule::~GeometryModule()
{
}

void GeometryModule::paint(juce::Graphics& g)
{
    Module::paint(g);
}

void GeometryModule::process()
{
    if (pins.at(0)->getConnections().size() == 1) {
        translate = pins.at(0)->getConnections().at(0)->getVector3D();
    }
    if (pins.at(1)->getConnections().size() == 1) {
        scale = pins.at(1)->getConnections().at(0)->getVector3D();
    }
    if (pins.at(2)->getConnections().size() == 1) {
        rotate = pins.at(2)->getConnections().at(0)->getVector3D();
    }

}

void GeometryModule::configurePins()
{
    Pin* p1 = new Pin(Pin::Type::VECTOR);
    p1->direction = Pin::Direction::IN;
    p1->setName("T");

    Pin* p2 = new Pin(Pin::Type::VECTOR);
    p2->direction = Pin::Direction::IN;
    p2->setName("R");

    Pin* p3 = new Pin(Pin::Type::VECTOR);
    p3->direction = Pin::Direction::IN;
    p3->setName("S");    

    addPin(p1->direction, p1);
    addPin(p2->direction, p2);
    addPin(p3->direction, p3);

}



