/*
  ==============================================================================
    MergeGeometryModule.cpp
    Created: 9 Oct 2025
    Author:  mpue
  ==============================================================================
*/

#include "MergeGeometryModule.h"

MergeGeometryModule::MergeGeometryModule()
{
    setSize(120, 140);
    setName("Merge");
    editable = false;
    prefab = true;
    setInterceptsMouseClicks(false, true);
}

MergeGeometryModule::~MergeGeometryModule()
{
    if (mergedGeometry != nullptr) {
        delete mergedGeometry;
    }
}

void MergeGeometryModule::paint(juce::Graphics& g)
{
    Module::paint(g);
}

void MergeGeometryModule::process()
{
    bool needsUpdate = false;

    // Check first geometry input
    if (pins.at(0)->getConnections().size() == 1) {
        Geometry* newGeom1 = pins.at(0)->getConnections().at(0)->getGeometry();
        if (geometry1 != newGeom1) {
            geometry1 = newGeom1;
            needsUpdate = true;
        }
    }
    else {
        if (geometry1 != nullptr) {
            geometry1 = nullptr;
            needsUpdate = true;
        }
    }

    // Check second geometry input
    if (pins.at(1)->getConnections().size() == 1) {
        Geometry* newGeom2 = pins.at(1)->getConnections().at(0)->getGeometry();
        if (geometry2 != newGeom2) {
            geometry2 = newGeom2;
            needsUpdate = true;
        }
    }
    else {
        if (geometry2 != nullptr) {
            geometry2 = nullptr;
            needsUpdate = true;
        }
    }

    // Merge geometries if both are available and update is needed
    if (needsUpdate) {
        mergeGeometries();
    }

    // Set output geometry
    if (mergedGeometry != nullptr && pins.at(2)->getConnections().size() > 0) {
        for (auto& connection : pins.at(2)->getConnections()) {
            connection->setGeometry(mergedGeometry);
        }
    }
}

void MergeGeometryModule::mergeGeometries()
{
    // Clean up previous merged geometry
    if (mergedGeometry != nullptr) {
        delete mergedGeometry;
        mergedGeometry = nullptr;
    }

    // If we don't have both geometries, return
    if (geometry1 == nullptr && geometry2 == nullptr) {
        return;
    }

    // If only one geometry is available, just pass it through
    if (geometry1 == nullptr) {
        int numVerts2 = geometry2->getNumVertices();
        int numIndices2 = geometry2->getNumIndices();

        float* newVertices = new float[numVerts2 * 6];
        unsigned int* newIndices = new unsigned int[numIndices2];

        std::memcpy(newVertices, geometry2->getVertices(), numVerts2 * 6 * sizeof(float));
        std::memcpy(newIndices, geometry2->getIndices(), numIndices2 * sizeof(unsigned int));

        mergedGeometry = new Geometry(newVertices, newIndices, numVerts2, numIndices2);
        return;
    }

    if (geometry2 == nullptr) {
        int numVerts1 = geometry1->getNumVertices();
        int numIndices1 = geometry1->getNumIndices();

        float* newVertices = new float[numVerts1 * 6];
        unsigned int* newIndices = new unsigned int[numIndices1];

        std::memcpy(newVertices, geometry1->getVertices(), numVerts1 * 6 * sizeof(float));
        std::memcpy(newIndices, geometry1->getIndices(), numIndices1 * sizeof(unsigned int));

        mergedGeometry = new Geometry(newVertices, newIndices, numVerts1, numIndices1);
        return;
    }

    // Both geometries available - merge them
    int numVerts1 = geometry1->getNumVertices();
    int numVerts2 = geometry2->getNumVertices();
    int totalVertices = numVerts1 + numVerts2;

    int numIndices1 = geometry1->getNumIndices();
    int numIndices2 = geometry2->getNumIndices();
    int totalIndices = numIndices1 + numIndices2;

    // Allocate new arrays for merged geometry
    // Each vertex has 6 floats (3 for position, 3 for color)
    float* newVertices = new float[totalVertices * 6];
    unsigned int* newIndices = new unsigned int[totalIndices];

    // Copy vertices from first geometry
    std::memcpy(newVertices, geometry1->getVertices(), numVerts1 * 6 * sizeof(float));

    // Copy vertices from second geometry
    std::memcpy(newVertices + (numVerts1 * 6), geometry2->getVertices(), numVerts2 * 6 * sizeof(float));

    // Copy indices from first geometry
    std::memcpy(newIndices, geometry1->getIndices(), numIndices1 * sizeof(unsigned int));

    // Copy indices from second geometry, adjusting them by the vertex offset
    unsigned int* geom2Indices = geometry2->getIndices();
    for (int i = 0; i < numIndices2; i++) {
        newIndices[numIndices1 + i] = geom2Indices[i] + numVerts1;
    }

    // Create merged geometry
    mergedGeometry = new Geometry(newVertices, newIndices, totalVertices, totalIndices);

	pins.at(2)->setGeometry(mergedGeometry);    
}

void MergeGeometryModule::configurePins()
{
    Pin* p1 = new Pin(Pin::Type::GEOMETRY);
    p1->direction = Pin::Direction::IN;
    p1->setName("Geom1");

    Pin* p2 = new Pin(Pin::Type::GEOMETRY);
    p2->direction = Pin::Direction::IN;
    p2->setName("Geom2");

    Pin* p3 = new Pin(Pin::Type::GEOMETRY);
    p3->direction = Pin::Direction::OUT;
    p3->setName("Out");

    addPin(p1->direction, p1);
    addPin(p2->direction, p2);
    addPin(p3->direction, p3);
}