#include "glew.h"		// include GL Extension Wrangler

#include "glfw3.h"  // include GLFW helper library

#include "glm.hpp"
#include "gtc/matrix_transform.hpp"
#include "gtx/rotate_vector.hpp"
#include "gtc/type_ptr.hpp"
#include "gtc/constants.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <vector>
#include <cctype>

// Ensure GLM uses radians
#define GLM_FORCE_RADIANS

using namespace std;

#define M_PI        3.14159265358979323846264338327950288   /* pi */
#define DEG_TO_RAD	M_PI/180.0f

#define CAMERA_SPEED -0.05f

GLFWwindow* window = 0x00;

GLuint shader_program = 0;

GLuint view_matrix_id = 0;
GLuint model_matrix_id = 0;
GLuint proj_matrix_id = 0;


///Transformations
glm::mat4 proj_matrix;
glm::mat4 view_matrix;
glm::mat4 model_matrix;

//Camera
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

GLfloat lastX = 800 / 2.0;
GLfloat lastY = 800 / 2.0;
GLfloat fov = 1.0f;

GLuint VBO, VAO, EBO;

GLfloat point_size = 3.0f;

bool moveMouse = false;

int sweep_type, nPointsBase, span;

vector<GLuint> indices;
vector<GLfloat> verticesBase, verticesTraj;

///Handle the keyboard input
void keyPressed(GLFWwindow *_window, int key, int scancode, int action, int mods) {
	switch (key) {
	// Rendering type
	case GLFW_KEY_W:
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		break;
	case GLFW_KEY_T:
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		break;
	case GLFW_KEY_P:
		glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
		break;
	// move handling (rotation)
	case GLFW_KEY_LEFT:
		model_matrix = glm::rotate(model_matrix, 0.05f, glm::vec3(0.0f, 0.05f, 0.0f));
		break;

	case GLFW_KEY_RIGHT:
		model_matrix = glm::rotate(model_matrix, -0.05f, glm::vec3(0.0f, 0.05f, 0.0f));
		break;
	case GLFW_KEY_UP:
		model_matrix = glm::rotate(model_matrix, 0.05f, glm::vec3(0.05f, 0.0f, 0.0f));
		break;
	case GLFW_KEY_DOWN:
		model_matrix = glm::rotate(model_matrix, -0.05f, glm::vec3(0.05f, 0.0f, 0.0f));
		break;
	// Close the window at ESC
	case GLFW_KEY_ESCAPE:
		glfwSetWindowShouldClose(window, GL_TRUE);
		break;
	}
	return;
}
/*
	If left mouse button is pressed, the user can zoom in and out by moving the mouse
*/
void mouse_button_callback(GLFWwindow *_window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		moveMouse = true;
	}
	else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
		moveMouse = false;
	}
}
// Mouse movement callback
bool firstMouse = true;	// First call
void mouse_callback(GLFWwindow* window, double xpos, double ypos){
	if (firstMouse)		// If this is the first call
	{	
		lastX = xpos;	// get initial x of mouse
		lastY = ypos;	// get initial y of mouse
		firstMouse = false;
	}
	if (moveMouse) {	// If left button is pressed
		GLfloat xoffset = xpos - lastX;		// calculate offset of mouse from last position
		GLfloat yoffset = lastY - ypos;
		lastX = xpos;
		lastY = ypos;

		GLfloat sensitivity = 0.01;	// Speed on which the zooming occurs
		xoffset *= sensitivity;
		yoffset *= sensitivity;
		fov -= yoffset;				// field of view changes 
	}
}
/*
	On window resizing the viewport is recalculated with the new dimensions
*/
void windowResizeCallback(GLFWwindow *_window, int width, int height) {
	glViewport(0, 0, width, height);
}

/**
	Initializes GLFW and GLEW
	Set the callback listeners
	Enable depth test
**/
bool initialize() {
	/// Initialize GL context and O/S window using the GLFW helper library
	if (!glfwInit()) {
		fprintf(stderr, "ERROR: could not start GLFW3\n");
		return false;
	}

	/// Create a window of size 640x480 and with title "Lecture 2: First Triangle"
	glfwWindowHint(GLFW_DOUBLEBUFFER, GL_TRUE);
	window = glfwCreateWindow(800, 800, "COMP371: Assignment 1", NULL, NULL);
	if (!window) {
		fprintf(stderr, "ERROR: could not open window with GLFW3\n");
		glfwTerminate();
		return false;
	}

	int w, h;
	glfwGetWindowSize(window, &w, &h);

	// EVENT CALLBACKS

	///Register the keyboard callback function: keyPressed(...)
	glfwSetKeyCallback(window, keyPressed);

	glfwSetCursorPosCallback(window, mouse_callback);

	glfwSetMouseButtonCallback(window, mouse_button_callback);

	glfwSetWindowSizeCallback(window, windowResizeCallback);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	//-------------------------------------------------

	glfwMakeContextCurrent(window);

	/// Initialize GLEW extension handler
	glewExperimental = GL_TRUE;	///Needed to get the latest version of OpenGL
	glewInit();

	/// Get the current OpenGL version
	const GLubyte* renderer = glGetString(GL_RENDERER); /// Get renderer string
	const GLubyte* version = glGetString(GL_VERSION); /// Version as a string
	printf("Renderer: %s\n", renderer);
	printf("OpenGL version supported %s\n", version);

	/// Enable the depth test i.e. draw a pixel if it's closer to the viewer
	glEnable(GL_DEPTH_TEST); /// Enable depth-testing
	glDepthFunc(GL_LESS);	/// The type of testing i.e. a smaller value as "closer"

	return true;
}
/*
	destoys application at closure
*/
bool cleanUp() {
	glDisableVertexAttribArray(0);
	//Properly de-allocate all resources once they've outlived their purpose
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);

	// Close GL context and any other GLFW resources
	glfwTerminate();

	return true;
}
/*
	Load shaders from given files and creates them
*/
GLuint loadShaders(std::string vertex_shader_path, std::string fragment_shader_path) {
	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_shader_path, std::ios::in);
	if (VertexShaderStream.is_open()) {
		std::string Line = "";
		while (getline(VertexShaderStream, Line))
			VertexShaderCode += "\n" + Line;
		VertexShaderStream.close();
	}
	else {
		printf("Impossible to open %s. Are you in the right directory ? Don't forget to read the FAQ !\n", vertex_shader_path.c_str());
		getchar();
		exit(-1);
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_shader_path, std::ios::in);
	if (FragmentShaderStream.is_open()) {
		std::string Line = "";
		while (getline(FragmentShaderStream, Line))
			FragmentShaderCode += "\n" + Line;
		FragmentShaderStream.close();
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	printf("Compiling shader : %s\n", vertex_shader_path.c_str());
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer, nullptr);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> VertexShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(VertexShaderID, InfoLogLength, nullptr, &VertexShaderErrorMessage[0]);
		printf("%s\n", &VertexShaderErrorMessage[0]);
	}

	// Compile Fragment Shader
	printf("Compiling shader : %s\n", fragment_shader_path.c_str());
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer, nullptr);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> FragmentShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(FragmentShaderID, InfoLogLength, nullptr, &FragmentShaderErrorMessage[0]);
		printf("%s\n", &FragmentShaderErrorMessage[0]);
	}

	// Link the program
	printf("Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);

	glBindAttribLocation(ProgramID, 0, "in_Position");

	//appearing in the vertex shader.
	glBindAttribLocation(ProgramID, 1, "in_Color");

	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
		glGetProgramInfoLog(ProgramID, InfoLogLength, nullptr, &ProgramErrorMessage[0]);
		printf("%s\n", &ProgramErrorMessage[0]);
	}

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	//The three variables below hold the id of each of the variables in the shader
	//If you read the vertex shader file you'll see that the same variable names are used.
	view_matrix_id = glGetUniformLocation(ProgramID, "view_matrix");
	model_matrix_id = glGetUniformLocation(ProgramID, "model_matrix");
	proj_matrix_id = glGetUniformLocation(ProgramID, "proj_matrix");

	return ProgramID;
}
/*
	read form (model) from a given file
*/
void readForm(string file) {
	// Read the Fragment Shader code from the file
	cout << "reading file" << endl;
	std::ifstream VertexReaderStream(file, std::ios::in);	// Creates stream with file
	if (VertexReaderStream.is_open()) {
		
		int rotSpan, nPoints;
		VertexReaderStream >> sweep_type;					// gets sweep type
		if (sweep_type != 0) {								// If rotation
			VertexReaderStream >> rotSpan;					// get rotation span
			span = rotSpan;									
			VertexReaderStream >> nPoints;					// gets number of points 
			cout << "points " << nPoints << endl;
			nPointsBase = nPoints * 3;						// saves number of coordenates
			for (int i = 0; i < nPointsBase; i++)
			{
				float pos;
				VertexReaderStream >> pos;					// read all vertices
				verticesBase.push_back(pos);
			}
		}
		else {												// If translation
			VertexReaderStream >> nPoints;					// number of points in base curve
			nPointsBase = nPoints * 3;
			for (int i = 0; i < nPoints * 3; i++)
			{
				float pos;
				VertexReaderStream >> pos;
				verticesBase.push_back(pos);
			}

			VertexReaderStream >> nPoints;					// Number of points in trajectory curve
			span = nPoints;
			for (int i = 0; i < nPoints * 3; i++)
			{
				float pos;
				VertexReaderStream >> pos;
				verticesTraj.push_back(pos);
			}
		}
		VertexReaderStream.close();							// Close the files
	}
}
/*
	build the indices vector for rotational sweep
	differs from the translation version because there 
	is one extra conection to close the	revolution surface

				0---2
				|  /|
				| / |
				1/--3
*/
void buildIndicesRotationSweep() {
	int pointsOriginal = (nPointsBase / 3);
	int totalPoints = span * pointsOriginal;
	for (int i = 0; i < span; i++)
	{
		for (int j = 0; j < pointsOriginal - 1; j++)
		{
			int h = (pointsOriginal*i) + j;
			indices.push_back(((pointsOriginal*i) + j) % totalPoints);
			h = (pointsOriginal*i) + j + 1;
			indices.push_back(((pointsOriginal*i) + j + 1) % totalPoints);
			h = (pointsOriginal*i) + j + pointsOriginal;
			indices.push_back(((pointsOriginal*i) + j + pointsOriginal) % totalPoints);
			indices.push_back(((pointsOriginal*i) + j + 1) % totalPoints);
			indices.push_back(((pointsOriginal*i) + j + pointsOriginal + 1) % totalPoints);
			indices.push_back(((pointsOriginal*i) + j + pointsOriginal) % totalPoints);
		}
	}
}
/*
build the indices vector for translation sweep
*/
void buildIndicesTranslationSweep() {
	int pointsOriginal = (nPointsBase / 3);
	int totalPoints = span * pointsOriginal;
	for (int i = 0; i < span-1; i++)
	{
		for (int j = 0; j < pointsOriginal - 1; j++)
		{
			int h = (pointsOriginal*i) + j;
			indices.push_back(((pointsOriginal*i) + j));
			h = (pointsOriginal*i) + j + 1;
			indices.push_back(((pointsOriginal*i) + j + 1));
			h = (pointsOriginal*i) + j + pointsOriginal;
			indices.push_back(((pointsOriginal*i) + j + pointsOriginal));
			indices.push_back(((pointsOriginal*i) + j + 1) % totalPoints);
			indices.push_back(((pointsOriginal*i) + j + pointsOriginal + 1));
			indices.push_back(((pointsOriginal*i) + j + pointsOriginal));
		}
	}
}
/*
	Method does sweep on the base vertices
*/
void buildForm() {
	if (sweep_type != 0) {							// Check if rotation or translation
		float rotationAngle = (float)360 / (span);	// 360 divided by number of rotations		
		for (int t = 0; t < (nPointsBase / 3)*(span-1); t++)
		{
			glm::vec4 v = glm::vec4(verticesBase[0 + (t*3)], verticesBase[1 + (t*3)], verticesBase[2 + (t*3)], 1.0);
			glm::mat4 rotationMatrix;
			rotationMatrix = glm::rotate(rotationMatrix, glm::radians(rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
			v = rotationMatrix * v;
			verticesBase.push_back(v.x);				// Build final vertices
			verticesBase.push_back(v.y);
			verticesBase.push_back(v.z);
		}
		buildIndicesRotationSweep();					// Get indices for Element array
	}
	else {
		// Translation Sweep
		// Translate base curve to trajectory point 0
		// copy base curve to each point in the trajectory curve

		// Move base to trajectory
		glm::vec3 distance;
		GLfloat x, y, z;
		z = verticesBase.back();				// Bottom point to top point
		verticesBase.pop_back();
		y = verticesBase.back();
		verticesBase.pop_back();
		x = verticesBase.back();
		verticesBase.pop_back();
		glm::vec3 basePoint = glm::vec3(x, y, z);
		glm::vec3 trajPoint = glm::vec3(verticesTraj[0], verticesTraj[1], verticesTraj[2]);
		distance = basePoint - trajPoint;		// vector that tells target of translation


		vector<float> v;

		v.push_back(verticesTraj[0]);
		v.push_back(verticesTraj[1]);
		v.push_back(verticesTraj[2]);

		for (int j = (verticesBase.size()); j >= 3; j -= 3)
		{
			GLfloat x2, y2, z2;
			z2 = verticesBase.back();
			verticesBase.pop_back();
			y2 = verticesBase.back();
			verticesBase.pop_back();
			x2 = verticesBase.back();
			verticesBase.pop_back();
			glm::vec3 transPoint = glm::vec3(x2, y2, z2);
			transPoint = transPoint - distance;
			v.push_back(transPoint.x);
			v.push_back(transPoint.y);
			v.push_back(transPoint.z);
		}

		basePoint = trajPoint;
		// for every point in traj
		for (int i = 3; i < verticesTraj.size(); i+=3)
		{
			trajPoint = glm::vec3(verticesTraj[i], verticesTraj[i+1], verticesTraj[i+2]);
			distance = basePoint - trajPoint;		// Get distance to new point
			v.push_back(verticesTraj[i]);
			v.push_back(verticesTraj[i + 1]);
			v.push_back(verticesTraj[i + 2]);
			// for points in base
			for (int j = 3; j < nPointsBase; j += 3)
			{
				// Translate base
				glm::vec3 transPoint = glm::vec3(v.at(j), v.at(j + 1), v.at(j+2));
				transPoint = transPoint - distance;		// translate
				v.push_back(transPoint.x);				// build vertices
				v.push_back(transPoint.y);
				v.push_back(transPoint.z);
			}

		}
		for (int i = 0; i < v.size(); i++)
		{
			verticesBase.push_back(v[i]);				// copy to global vector
		}
		buildIndicesTranslationSweep();					// build indices
	}
}




int main() {

	initialize();
	readForm("form.txt");			// Read form file

	buildForm();					// Build solid

	///Load the shaders
	shader_program = loadShaders("COMP371_hw1.vs", "COMP371_hw1.fs");

	// This will identify our vertex buffer
	GLuint vertexbuffer;

	// Generate 1 buffer, put the resulting identifier in vertexbuffer
	glGenBuffers(1, &vertexbuffer);

	// The following commands will talk about our 'vertexbuffer' buffer
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);

	glGenBuffers(1, &EBO);
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	
	// Give our vertices to OpenGL.
	glBufferData(GL_ARRAY_BUFFER, sizeof(GL_FLOAT)*verticesBase.size(), &verticesBase[0], GL_STATIC_DRAW);
	//glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GL_FLOAT)*indices.size(), &indices[0], GL_STATIC_DRAW);
	glVertexAttribPointer(
		0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
		3,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		0,                  // stride
		(void*)0            // array buffer offset
		);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);

	

	while (!glfwWindowShouldClose(window)) {
		// wipe the drawing surface clear
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(0.1f, 0.2f, 0.2f, 1.0f);
		glPointSize(point_size);

		glUseProgram(shader_program);


		view_matrix = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
		proj_matrix = glm::perspective(fov, (GLfloat)800 / (GLfloat)800, 0.1f, 100.0f);


		//Pass the values of the three matrices to the shaders
		glUniformMatrix4fv(proj_matrix_id, 1, GL_FALSE, glm::value_ptr(proj_matrix));
		glUniformMatrix4fv(view_matrix_id, 1, GL_FALSE, glm::value_ptr(view_matrix));
		glUniformMatrix4fv(model_matrix_id, 1, GL_FALSE, glm::value_ptr(model_matrix));

		glBindVertexArray(VAO);
		// Draw the triangle
		glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, &indices[0]);

		glBindVertexArray(0);

		// update other events like input handling
		glfwPollEvents();
		// put the stuff we've been drawing onto the display
		glfwSwapBuffers(window);
	}

	cleanUp();
	return 0;
}