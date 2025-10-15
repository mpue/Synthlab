/*
  ==============================================================================

    GeometryModule.cpp
    Created: 7 Oct 2025 11:09:10am
    Author:  mpue

  ==============================================================================
*/

#include "GeometryModule.h"
#include "Graphics/Geometry.h"

GeometryModule::GeometryModule()
{    
    setSize(120, 140);
    setName("GeometryView");
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
    if (pins.at(3)->getConnections().size() == 1) {
		if (geometry != pins.at(3)->getConnections().at(0)->getGeometry()) {
            geometry = pins.at(3)->getConnections().at(0)->getGeometry();		
        }
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

    Pin* p4 = new Pin(Pin::Type::GEOMETRY);
    p4->direction = Pin::Direction::IN;
    p4->setName("Geom");

    addPin(p1->direction, p1);
    addPin(p2->direction, p2);
    addPin(p3->direction, p3);
    addPin(p4->direction, p4);
}



