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

	Geometry(float p_vertices[], unsigned int p_indices[], int numVertices, int numIndices) : 
		vertices(p_vertices), indices(p_indices), numVertices(numVertices),numIndices(numIndices) {
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

	void setNumVertices(int num) {
		this->numVertices = num;
	}

	int getNumVertices() {
		return numVertices;
	}

	void setNumIndices(int num) {
		this->numIndices = num;
	}

	int getNumIndices() {
		return numIndices;
	}

private:

	int numVertices;
	int numIndices;

	float* vertices;
	unsigned int* indices;

};