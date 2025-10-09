/*
  ==============================================================================
    TransformGeometryModule.cpp
    Created: 9 Oct 2025
    Author:  mpue
  ==============================================================================
*/

#include "TransformGeometryModule.h"

TransformGeometryModule::TransformGeometryModule()
{
    setSize(120, 140);
    setName("Transform");
    editable = false;
    prefab = true;
    setInterceptsMouseClicks(false, true);
}

TransformGeometryModule::~TransformGeometryModule()
{
    if (transformedGeometry != nullptr) {
        delete transformedGeometry;
    }
}

void TransformGeometryModule::paint(juce::Graphics& g)
{
    Module::paint(g);
}

void TransformGeometryModule::process()
{
    bool needsUpdate = false;

    // Check input geometry
    if (pins.at(0)->getConnections().size() == 1) {
        Geometry* newGeom = pins.at(0)->getConnections().at(0)->getGeometry();
        if (inputGeometry != newGeom) {
            inputGeometry = newGeom;
            needsUpdate = true;
        }
    }
    else {
        if (inputGeometry != nullptr) {
            inputGeometry = nullptr;
            needsUpdate = true;
        }
    }

    // Check translate vector
    if (pins.at(1)->getConnections().size() == 1) {
        Vector3D<float> newTranslate = pins.at(1)->getConnections().at(0)->getVector3D();
        if (translate.x != newTranslate.x || translate.y != newTranslate.y || translate.z != translate.z) {
            translate = newTranslate;
            needsUpdate = true;
        }
    }
    else {
        if (translate.normalised().length() > 0) {
            translate = { 0.0f, 0.0f, 0.0f };
            needsUpdate = true;
        }
    }

    // Check rotate vector
    if (pins.at(2)->getConnections().size() == 1) {
        Vector3D<float> newRotate = pins.at(2)->getConnections().at(0)->getVector3D();
        if (rotate.x != newRotate.x || rotate.y != newRotate.y || rotate.z != newRotate.z) {
            rotate = newRotate;
            needsUpdate = true;
        }
    }
    else {
        if (rotate.x != 0 || rotate.y != 0 || rotate.z != 0) {
            rotate = { 0.0f, 0.0f, 0.0f };
            needsUpdate = true;
        }
    }

    // Check scale vector
    if (pins.at(3)->getConnections().size() == 1) {
        Vector3D<float> newScale = pins.at(3)->getConnections().at(0)->getVector3D();
        if (scale.x != newScale.x || scale.y != newScale.y || scale.z != newScale.z ) {
            scale = newScale;
            needsUpdate = true;
        }
    }
    else {
        if (scale.x != 1.0f || scale.y != 1.0f || scale.z != 1.0f) {
            scale = { 1.0f, 1.0f, 1.0f };
            needsUpdate = true;
        }
    }

    // Transform geometry if needed
    if (needsUpdate && inputGeometry != nullptr) {
        transformGeometry();
    }

    // Set output geometry
	pins.at(4)->setGeometry(transformedGeometry);   
}

Matrix3D<float> TransformGeometryModule::createRotationMatrix(const Vector3D<float>& rotation)
{
    // Convert degrees to radians
    float rx = rotation.x * MathConstants<float>::pi / 180.0f;
    float ry = rotation.y * MathConstants<float>::pi / 180.0f;
    float rz = rotation.z * MathConstants<float>::pi / 180.0f;

    // Create rotation matrices for each axis
    Matrix3D<float> rotX = Matrix3D<float>::rotation({ rx, 0.0f, 0.0f });
    Matrix3D<float> rotY = Matrix3D<float>::rotation({ 0.0f, ry, 0.0f });
    Matrix3D<float> rotZ = Matrix3D<float>::rotation({ 0.0f, 0.0f, rz });

    // Combine rotations: Z * Y * X
    return rotZ * rotY * rotX;
}

Vector3D<float> TransformGeometryModule::transformVertex(const Vector3D<float>& vertex,
    const Vector3D<float>& translation,
    const Vector3D<float>& rotation,
    const Vector3D<float>& scale)
{
    // Apply transformations in order: Scale -> Rotate -> Translate
    Vector3D<float> transformed = vertex;

    // 1. Scale
    transformed.x *= scale.x;
    transformed.y *= scale.y;
    transformed.z *= scale.z;

    // 2. Rotate
    Matrix3D<float> rotationMatrix = createRotationMatrix(rotation);
    Vector3D<float> rotated;
    rotated.x = rotationMatrix.mat[0] * transformed.x +
        rotationMatrix.mat[4] * transformed.y +
        rotationMatrix.mat[8] * transformed.z;
    rotated.y = rotationMatrix.mat[1] * transformed.x +
        rotationMatrix.mat[5] * transformed.y +
        rotationMatrix.mat[9] * transformed.z;
    rotated.z = rotationMatrix.mat[2] * transformed.x +
        rotationMatrix.mat[6] * transformed.y +
        rotationMatrix.mat[10] * transformed.z;

    // 3. Translate
    rotated.x += translation.x;
    rotated.y += translation.y;
    rotated.z += translation.z;

    return rotated;
}

void TransformGeometryModule::transformGeometry()
{
    // Clean up previous transformed geometry
    if (transformedGeometry != nullptr) {
        delete transformedGeometry;
        transformedGeometry = nullptr;
    }

    if (inputGeometry == nullptr) {
        return;
    }

    int numVertices = inputGeometry->getNumVertices();
    int numIndices = inputGeometry->getNumIndices();

    // Allocate new arrays for transformed geometry
    // Each vertex has 6 floats (3 for position, 3 for color)
    float* newVertices = new float[numVertices * 6];
    unsigned int* newIndices = new unsigned int[numIndices];

    float* originalVertices = inputGeometry->getVertices();

    // Transform each vertex
    for (int i = 0; i < numVertices; i++) {
        int baseIndex = i * 6;

        // Get original position
        Vector3D<float> position(
            originalVertices[baseIndex + 0],
            originalVertices[baseIndex + 1],
            originalVertices[baseIndex + 2]
        );

        // Transform position
        Vector3D<float> transformedPos = transformVertex(position, translate, rotate, scale);

        // Set transformed position
        newVertices[baseIndex + 0] = transformedPos.x;
        newVertices[baseIndex + 1] = transformedPos.y;
        newVertices[baseIndex + 2] = transformedPos.z;

        // Copy color unchanged
        newVertices[baseIndex + 3] = originalVertices[baseIndex + 3];
        newVertices[baseIndex + 4] = originalVertices[baseIndex + 4];
        newVertices[baseIndex + 5] = originalVertices[baseIndex + 5];
    }

    // Copy indices unchanged
    std::memcpy(newIndices, inputGeometry->getIndices(), numIndices * sizeof(unsigned int));

    // Create transformed geometry
    transformedGeometry = new Geometry(newVertices, newIndices, numVertices, numIndices);

    // Notify listeners that geometry has changed
    juce::ChangeBroadcaster::sendChangeMessage();
}

void TransformGeometryModule::configurePins()
{
    Pin* p1 = new Pin(Pin::Type::GEOMETRY);
    p1->direction = Pin::Direction::IN;
    p1->setName("Geom");

    Pin* p2 = new Pin(Pin::Type::VECTOR);
    p2->direction = Pin::Direction::IN;
    p2->setName("T");

    Pin* p3 = new Pin(Pin::Type::VECTOR);
    p3->direction = Pin::Direction::IN;
    p3->setName("R");

    Pin* p4 = new Pin(Pin::Type::VECTOR);
    p4->direction = Pin::Direction::IN;
    p4->setName("S");

    Pin* p5 = new Pin(Pin::Type::GEOMETRY);
    p5->direction = Pin::Direction::OUT;
    p5->setName("Out");

    addPin(p1->direction, p1);
    addPin(p2->direction, p2);
    addPin(p3->direction, p3);
    addPin(p4->direction, p4);
    addPin(p5->direction, p5);
}