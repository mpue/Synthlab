/*
  ==============================================================================

    Geometry.h
    Created: 7 Oct 2025 2:45:30pm
    Author:  mpue

  ==============================================================================
*/

#pragma once

class Geometry {

public:

    Geometry(float p_vertices[], unsigned int p_indices[]) : vertices(p_vertices), indices(p_indices){
    }

    ~Geometry() {
        delete[] vertices;
		delete[] indices;
	}

    unsigned int* getIndices() {
        return indices;
	}

    float* getVertices() {
        return vertices;     
    }

private:

    float* vertices;
    unsigned int* indices;

};