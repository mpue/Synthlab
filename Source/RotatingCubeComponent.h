#pragma once

#include <JuceHeader.h>
#include "GeometryModule.h"

using namespace juce;
using namespace juce::gl;

class RotatingCubeComponent : public juce::OpenGLAppComponent, public juce::Timer, public juce::ChangeListener
{
public:

	RotatingCubeComponent() {
		setSize(800, 600);
	}

	RotatingCubeComponent(GeometryModule* m)
	{
		this->module = m;
		setSize(800, 600);
	}

	~RotatingCubeComponent() override
	{
		shutdownOpenGL();
	}

	void timerCallback() override
	{
		updateRotation();
	}

	virtual void changeListenerCallback(juce::ChangeBroadcaster* source) override
	{
		GeometryModule* m = dynamic_cast<GeometryModule*>(module);

		if (m != nullptr) {
			setModule(m);
			bindBufferData();
		}
	}

	void setModule(GeometryModule* m) {
		if (module != nullptr) {
			module->removeChangeListener(this);
		}
		this->module = m;
		module->addChangeListener(this);
	}

	void initShaders() {
		// Shader-Source-Code
		const char* vertexShader = R"(
            #version 330 core
            layout (location = 0) in vec3 position;
            layout (location = 1) in vec3 color;
            
            out vec3 fragColor;
            
            uniform mat4 projection;
            uniform mat4 view;
            uniform mat4 model;
            
            void main()
            {
                gl_Position = projection * view * model * vec4(position, 1.0);
                fragColor = color;
            }
        )";

		const char* fragmentShader = R"(
            #version 330 core
            in vec3 fragColor;
            out vec4 color;
            
            void main()
            {
                color = vec4(fragColor, 1.0);
            }
        )";

		// Shader kompilieren
		std::unique_ptr<juce::OpenGLShaderProgram> newShader(new juce::OpenGLShaderProgram(openGLContext));

		if (newShader->addVertexShader(vertexShader) &&
			newShader->addFragmentShader(fragmentShader) &&
			newShader->link())
		{
			shader.reset(newShader.release());
			shader->use();
		}
	}

	void bindBufferData() {

		float* vertices = module->getGeometry()->getVertices();
		unsigned int* indices = module->getGeometry()->getIndices();

		// Speichere die Anzahl der Indizes für das Rendering
		numIndicesToDraw = module->getGeometry()->getNumIndices();
		int numVertexFloats = module->getGeometry()->getNumVertices() * 6; // 6 floats pro Vertex (pos + color)

		// VAO, VBO, EBO erstellen
		openGLContext.extensions.glGenVertexArrays(1, &VAO);
		openGLContext.extensions.glGenBuffers(1, &VBO);
		openGLContext.extensions.glGenBuffers(1, &EBO);

		openGLContext.extensions.glBindVertexArray(VAO);

		openGLContext.extensions.glBindBuffer(GL_ARRAY_BUFFER, VBO);
		openGLContext.extensions.glBufferData(GL_ARRAY_BUFFER, numVertexFloats * sizeof(float), vertices, GL_STATIC_DRAW);

		openGLContext.extensions.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		openGLContext.extensions.glBufferData(GL_ELEMENT_ARRAY_BUFFER, numIndicesToDraw * sizeof(unsigned int), indices, GL_STATIC_DRAW);

		// Position-Attribut
		openGLContext.extensions.glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
		openGLContext.extensions.glEnableVertexAttribArray(0);

		// Farb-Attribut
		openGLContext.extensions.glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
		openGLContext.extensions.glEnableVertexAttribArray(1);

		openGLContext.extensions.glBindVertexArray(0);

		// Depth-Test und Back-Face Culling aktivieren
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glFrontFace(GL_CCW);

	}

	void initialise() override
	{
		initShaders();
		bindBufferData();
	}

	void shutdown() override
	{
		shader.reset();
		openGLContext.extensions.glDeleteVertexArrays(1, &VAO);
		openGLContext.extensions.glDeleteBuffers(1, &VBO);
		openGLContext.extensions.glDeleteBuffers(1, &EBO);
	}

	void render() override
	{
		jassert(juce::OpenGLHelpers::isContextActive());

		const float desktopScale = (float)openGLContext.getRenderingScale();

		// Depth Buffer und Color Buffer clearen
		glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glViewport(0, 0, juce::roundToInt(desktopScale * getWidth()),
			juce::roundToInt(desktopScale * getHeight()));

		if (shader == nullptr)
			return;

		shader->use();

		// Matrizen erstellen
		float aspect = getWidth() / (float)getHeight();

		// Projection Matrix
		juce::Matrix3D<float> projection = createPerspectiveMatrix(45.0f, aspect, 0.1f, 100.0f);

		// View Matrix
		juce::Matrix3D<float> view;
		view = view * juce::Matrix3D<float>::fromTranslation({ 0.0f, 0.0f, -3.0f });

		// Model Matrix (Rotation)
		juce::Matrix3D<float> model;
		model = model * juce::Matrix3D<float>::fromTranslation(translation) * juce::Matrix3D<float>::rotation(rotation);

		Matrix3D<float> scaleMatrix(
			scale.x, 0, 0, 0,
			0, scale.y, 0, 0,
			0, 0, scale.z, 0,
			0, 0, 0, 1
		);

		model = model * scaleMatrix;

		// Uniforms setzen
		GLint projLoc = openGLContext.extensions.glGetUniformLocation(shader->getProgramID(), "projection");
		openGLContext.extensions.glUniformMatrix4fv(projLoc, 1, GL_FALSE, projection.mat);

		GLint viewLoc = openGLContext.extensions.glGetUniformLocation(shader->getProgramID(), "view");
		openGLContext.extensions.glUniformMatrix4fv(viewLoc, 1, GL_FALSE, view.mat);

		GLint modelLoc = openGLContext.extensions.glGetUniformLocation(shader->getProgramID(), "model");
		openGLContext.extensions.glUniformMatrix4fv(modelLoc, 1, GL_FALSE, model.mat);

		// Geometrie mit korrekter Index-Anzahl zeichnen
		openGLContext.extensions.glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, numIndicesToDraw, GL_UNSIGNED_INT, 0);
		openGLContext.extensions.glBindVertexArray(0);
	}

	void updateRotation()
	{
		rotation = Vector3D<float>(0, 0, 0);
		if (module != nullptr) {
			if (module->getPins().at(0)->getConnections().size() == 1) {
				translation = module->getPins().at(0)->getConnections().at(0)->getVector3D();
			}
			if (module->getPins().at(1)->getConnections().size() == 1) {
				rotation = module->getPins().at(1)->getConnections().at(0)->getVector3D();
			}
			if (module->getPins().at(2)->getConnections().size() == 1) {
				scale = module->getPins().at(2)->getConnections().at(0)->getVector3D();
			}

		}
		repaint();
	}

private:
	juce::Matrix3D<float> createPerspectiveMatrix(float fov, float aspect, float near, float far)
	{
		float const tanHalfFovy = std::tan(fov / 2.0f * juce::MathConstants<float>::pi / 180.0f);

		juce::Matrix3D<float> result;
		result.mat[0] = 1.0f / (aspect * tanHalfFovy);
		result.mat[5] = 1.0f / tanHalfFovy;
		result.mat[10] = -(far + near) / (far - near);
		result.mat[11] = -1.0f;
		result.mat[14] = -(2.0f * far * near) / (far - near);
		result.mat[15] = 0.0f;

		return result;
	}

	std::unique_ptr<juce::OpenGLShaderProgram> shader;
	GLuint VAO = 0, VBO = 0, EBO = 0;

	Vector3D<float> translation = { 0.0f, 0.0f, 0.0f };
	Vector3D<float> rotation = { 0.0f, 0.0f, 0.0f };
	Vector3D<float> scale = { 1.0f, 1.0f, 1.0f };

	GeometryModule* module = nullptr;
	int numIndicesToDraw = 0;  // Speichert die Anzahl der zu zeichnenden Indizes

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RotatingCubeComponent)
};