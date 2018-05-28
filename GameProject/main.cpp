// Library for OpenGL function loading
// Must be included before GLFW
#define GLEW_STATIC
#include <GL/glew.h>

// Library for window creation and event handling
#include <GLFW/glfw3.h>

// Library for vertex and matrix math
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Library for loading .OBJ model
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

// Library for loading an image
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// Header for camera structure/functions
#include "camera.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "loadppm.h"
#include "Vec3D.h"
#include "mesh.h"
#include "grid.h"

// Configuration
const int WIDTH = 800;
const int HEIGHT = 600;

int camSelect = 0; // 0 for maincam, 1 for lightcam

Mesh mesh;
Mesh simplified;
Grid grid;

// Per-vertex data
struct Vertexx {
	glm::vec3 pos;
	glm::vec3 normal;
};

struct Light {
	glm::vec4 position;
	glm::vec3 intensities;
	float attenuation;
	float ambientCoefficient;
	float coneAngle;    // new for spotlight
	glm::vec3 coneDirection; // new for spotlight
};

void init(const char * fileName) {
	mesh.loadMesh(fileName);
	//mesh.computeBoundingCube();
	//attention! while loadMesh calls compute bounding box, it is not yet implemented!

}

// Helper function to read a file like a shader
std::string readFile(const std::string& path) {
	std::ifstream file(path, std::ios::binary);

	std::stringstream buffer;
	buffer << file.rdbuf();

	return buffer.str();
}

bool checkShaderErrors(GLuint shader) {
	// Check if the shader compiled successfully
	GLint compileSuccessful;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compileSuccessful);

	// If it didn't, then read and print the compile log
	if (!compileSuccessful) {
		GLint logLength;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);

		std::vector<GLchar> logBuffer(logLength);
		glGetShaderInfoLog(shader, logLength, nullptr, logBuffer.data());

		std::cerr << logBuffer.data() << std::endl;

		return false;
	}
	else {
		return true;
	}
}

bool checkProgramErrors(GLuint program) {
	// Check if the program linked successfully
	GLint linkSuccessful;
	glGetProgramiv(program, GL_LINK_STATUS, &linkSuccessful);

	// If it didn't, then read and print the link log
	if (!linkSuccessful) {
		GLint logLength;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);

		std::vector<GLchar> logBuffer(logLength);
		glGetProgramInfoLog(program, logLength, nullptr, logBuffer.data());

		std::cerr << logBuffer.data() << std::endl;

		return false;
	}
	else {
		return true;
	}
}

// OpenGL debug callback
void APIENTRY debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
	if (severity != GL_DEBUG_SEVERITY_NOTIFICATION) {
		std::cerr << "OpenGL: " << message << std::endl;
	}
}




// Key handle function
void keyboardHandler(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	cameraKeyboardHandler(key, action);

	switch (key)
	{
	case GLFW_KEY_1:
		camSelect = 0;
		//updateCamera(mainCamera);
		break;
	case GLFW_KEY_2:
		camSelect = 1;
		//updateCamera(lightCamera);
		break;
	case GLFW_KEY_7:
		simplified = grid.simplifyMesh(mesh, 30);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		simplified.drawSmooth();
		//updateCamera(lightCamera);
		break;
	default:
		break;
	}
}

// Mouse button handle function
void mouseButtonHandler(GLFWwindow* window, int button, int action, int mods)
{
	camMouseButtonHandler(button, action);
}

void cursorPosHandler(GLFWwindow* window, double xpos, double ypos)
{
	camCursorPosHandler(xpos, ypos);
}


int main() {
	
	init("Boss_high.obj");

	if (!glfwInit()) {
		std::cerr << "Failed to initialize GLFW!" << std::endl;
		return EXIT_FAILURE;
	}

	//////////////////// Create window and OpenGL 4.3 debug context
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Shadow mapping practical", nullptr, nullptr);
	if (!window) {
		std::cerr << "Failed to create OpenGL context!" << std::endl;
		std::cout << "Press enter to close."; getchar();
		return EXIT_FAILURE;
	}

	glfwSetKeyCallback(window, keyboardHandler);
	glfwSetMouseButtonCallback(window, mouseButtonHandler);
	glfwSetCursorPosCallback(window, cursorPosHandler);

	// Activate the OpenGL context
	glfwMakeContextCurrent(window);

	// Initialize GLEW extension loader
	glewExperimental = GL_TRUE;
	glewInit();

	// Set up OpenGL debug callback
	glDebugMessageCallback(debugCallback, nullptr);

	GLuint mainProgram = glCreateProgram();
	GLuint shadowProgram = glCreateProgram();


	////////////////// Load and compile main shader program
	{
		std::string vertexShaderCode = readFile("shader.vert");
		const char* vertexShaderCodePtr = vertexShaderCode.data();

		GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertexShader, 1, &vertexShaderCodePtr, nullptr);
		glCompileShader(vertexShader);

		std::string fragmentShaderCode = readFile("shader.frag");
		const char* fragmentShaderCodePtr = fragmentShaderCode.data();

		GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragmentShader, 1, &fragmentShaderCodePtr, nullptr);
		glCompileShader(fragmentShader);

		if (!checkShaderErrors(vertexShader) || !checkShaderErrors(fragmentShader)) {
			std::cerr << "Shader(s) failed to compile!" << std::endl;
			std::cout << "Press enter to close."; getchar();
			return EXIT_FAILURE;
		}

		// Combine vertex and fragment shaders into single shader program
		glAttachShader(mainProgram, vertexShader);
		glAttachShader(mainProgram, fragmentShader);
		glLinkProgram(mainProgram);

		if (!checkProgramErrors(mainProgram)) {
			std::cerr << "Main program failed to link!" << std::endl;
			std::cout << "Press enter to close."; getchar();
			return EXIT_FAILURE;
		}
	}

	////////////////// Load and compile shadow shader program
	{
		std::string vertexShaderCode = readFile("shadow.vert");
		const char* vertexShaderCodePtr = vertexShaderCode.data();

		GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertexShader, 1, &vertexShaderCodePtr, nullptr);
		glCompileShader(vertexShader);

		std::string fragmentShaderCode = readFile("shadow.frag");
		const char* fragmentShaderCodePtr = fragmentShaderCode.data();

		GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragmentShader, 1, &fragmentShaderCodePtr, nullptr);
		glCompileShader(fragmentShader);

		if (!checkShaderErrors(vertexShader) || !checkShaderErrors(fragmentShader)) {
			std::cerr << "Shader(s) failed to compile!" << std::endl;
			return EXIT_FAILURE;
		}

		// Combine vertex and fragment shaders into single shader program
		glAttachShader(shadowProgram, vertexShader);
		glAttachShader(shadowProgram, fragmentShader);
		glLinkProgram(shadowProgram);

		if (!checkProgramErrors(shadowProgram)) {
			std::cerr << "Shadow program failed to link!" << std::endl;
			return EXIT_FAILURE;
		}
	}

	//////////////////// Load a texture for exercise 5
	// Create Texture
	int texwidth, texheight, texchannels;
	stbi_uc* pixels = stbi_load("smiley.png", &texwidth, &texheight, &texchannels, 3);

	GLuint texLight;
	glGenTextures(1, &texLight);
	glBindTexture(GL_TEXTURE_2D, texLight);

	// Upload pixels into texture
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texwidth, texheight, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);

	// Set behaviour for when texture coordinates are outside the [0, 1] range
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Set interpolation for texture sampling (GL_NEAREST for no interpolation)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	////////////////////////// Load vertices of model
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, "torus.obj")) {
		std::cerr << err << std::endl;
		return EXIT_FAILURE;
	}

	tinyobj::attrib_t attrib1;
	std::vector<tinyobj::shape_t> shapes1;
	std::vector<tinyobj::material_t> materials1;
	std::string err1;

	if (!tinyobj::LoadObj(&attrib1, &shapes1, &materials1, &err1, "terrain.obj")) {
		std::cerr << err << std::endl;
		return EXIT_FAILURE;
	}


	std::vector<Vertexx> vertices;

	// Read triangle vertices from OBJ file
	for (const auto& shape : shapes) {
		for (const auto& index : shape.mesh.indices) {
			Vertexx vertex = {};

			// Retrieve coordinates for vertex by index
			vertex.pos = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			// Retrieve components of normal by index
			vertex.normal = {
				attrib.normals[3 * index.normal_index + 0],
				attrib.normals[3 * index.normal_index + 1],
				attrib.normals[3 * index.normal_index + 2]
			};

			vertices.push_back(vertex);
		}
	}

	for (const auto& shape : shapes1) {
		for (const auto& index : shape.mesh.indices) {
			Vertexx vertex = {};

			// Retrieve coordinates for vertex by index
			vertex.pos = {
				attrib1.vertices[3 * index.vertex_index + 0],
				attrib1.vertices[3 * index.vertex_index + 1],
				attrib1.vertices[3 * index.vertex_index + 2]
			};

			// Retrieve components of normal by index
			vertex.normal = {
				attrib1.normals[3 * index.normal_index + 0],
				attrib1.normals[3 * index.normal_index + 1],
				attrib1.normals[3 * index.normal_index + 2]
			};

			vertices.push_back(vertex);
		}
	}

	//////////////////// Create Vertex Buffer Object
	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertexx), vertices.data(), GL_STATIC_DRAW);

	// Bind vertex data to shader inputs using their index (location)
	// These bindings are stored in the Vertex Array Object
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// The position vectors should be retrieved from the specified Vertex Buffer Object with given offset and stride
	// Stride is the distance in bytes between vertices
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertexx), reinterpret_cast<void*>(offsetof(Vertexx, pos)));
	glEnableVertexAttribArray(0);

	// The normals should be retrieved from the same Vertex Buffer Object (glBindBuffer is optional)
	// The offset is different and the data should go to input 1 instead of 0
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertexx), reinterpret_cast<void*>(offsetof(Vertexx, normal)));
	glEnableVertexAttribArray(1);

	//////////////////// Create Shadow Texture
	GLuint texShadow;
	const int SHADOWTEX_WIDTH = 1024;
	const int SHADOWTEX_HEIGHT = 1024;
	glGenTextures(1, &texShadow);
	glBindTexture(GL_TEXTURE_2D, texShadow);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, SHADOWTEX_WIDTH, SHADOWTEX_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

	// Set behaviour for when texture coordinates are outside the [0, 1] range
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Set interpolation for texture sampling (GL_NEAREST for no interpolation)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	//////////////////// Create framebuffer for extra texture
	GLuint framebuffer;
	glGenFramebuffers(1, &framebuffer);

	/////////////////// Set shadow texure as depth buffer for this framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, texShadow, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	/////////////////// Create main camera
	Camera mainCamera;
	mainCamera.aspect = WIDTH / (float)HEIGHT;
	mainCamera.position = glm::vec3(1.2f, 1.1f, 0.9f);
	mainCamera.forward = -mainCamera.position;

	Camera lightCamera;
	lightCamera.aspect = WIDTH / (float)HEIGHT;
	lightCamera.position = glm::vec4(0.0f, 3.0f, 10.0f, 1.0f);
	lightCamera.forward = -lightCamera.position;

	Camera cam = mainCamera;




	// Main loop
	while (!glfwWindowShouldClose(window)) {

		glfwPollEvents();



		//////// Stub code for you to fill in order to render the shadow map
		{
			// Bind the off-screen framebuffer
			glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
			
			// Clear the shadow map and set needed options
			glClearDepth(1.0f);
			glClear(GL_DEPTH_BUFFER_BIT);
			glEnable(GL_DEPTH_TEST);

			// Bind the shader
			glUseProgram(shadowProgram);

			// Set viewport size
			glViewport(0, 0, SHADOWTEX_WIDTH, SHADOWTEX_HEIGHT);

			// Execute draw command
			glDrawArrays(GL_TRIANGLES, 0, vertices.size());

			
			// .... HERE YOU MUST ADD THE CORRECT UNIFORMS FOR RENDERING THE SHADOW MAP
			glm::mat4 lightMVP = lightCamera.vpMatrix();
			glUniformMatrix4fv(glGetUniformLocation(shadowProgram, "mvp"), 1, GL_FALSE, glm::value_ptr(lightMVP));


			// Bind vertex data
			glBindVertexArray(vao);

			// Execute draw command
			glDrawArrays(GL_TRIANGLES, 0, vertices.size());

			// Unbind the off-screen framebuffer
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		// Bind the shader
		glUseProgram(mainProgram);

		if (camSelect == 0) {
			cam = mainCamera;
			updateCamera(mainCamera);
		}
		else if (camSelect == 1) {
			cam = lightCamera;
			updateCamera(lightCamera);
		}

		//updateCamera(cam);

		glm::mat4 biasMatrix(
			0.5, 0.0, 0.0, 0.0,
			0.0, 0.5, 0.0, 0.0,
			0.0, 0.0, 0.5, 0.0,
			0.5, 0.5, 0.5, 1.0
		);
		glm::mat4 depthMVP = lightCamera.vpMatrix();
		glm::mat4 depthBiasMVP = biasMatrix * depthMVP;

		glm::mat4 mvp = cam.vpMatrix();

		glUniformMatrix4fv(glGetUniformLocation(mainProgram, "mvp"), 1, GL_FALSE, glm::value_ptr(mvp));

		glUniformMatrix4fv(glGetUniformLocation(mainProgram, "lightMVP"), 1, GL_FALSE, glm::value_ptr(depthBiasMVP));

		// Set view position
		glUniform3fv(glGetUniformLocation(mainProgram, "viewPos"), 1, glm::value_ptr(cam.position));

		// Expose current time in shader uniform
		glUniform1f(glGetUniformLocation(mainProgram, "time"), static_cast<float>(glfwGetTime()));

		//lightpos in shader uniform
		glUniform3fv(glGetUniformLocation(mainProgram, "lightPos"), 1, glm::value_ptr(lightCamera.position));

		// Bind vertex data
		glBindVertexArray(vao);

		// Bind the shadow map to texture slot 0
		GLint texture_unit = 0;
		glActiveTexture(GL_TEXTURE0 + texture_unit);
		glBindTexture(GL_TEXTURE_2D, texShadow);
		glUniform1i(glGetUniformLocation(mainProgram, "texShadow"), texture_unit);

		glActiveTexture(GL_TEXTURE0 + 1);
		glBindTexture(GL_TEXTURE_2D, texLight);
		glUniform1i(glGetUniformLocation(mainProgram, "texLight"), 1);

		// Set viewport size
		glViewport(0, 0, WIDTH, HEIGHT);

		// Clear the framebuffer to black and depth to maximum value
		glClearDepth(1.0f);
		glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDisable(GL_CULL_FACE);
		glEnable(GL_DEPTH_TEST);

		// Execute draw command
		glDrawArrays(GL_TRIANGLES, 0, vertices.size());

		// Present result to the screen
		glfwSwapBuffers(window);
	}

	glDeleteFramebuffers(1, &framebuffer);

	glDeleteTextures(1, &texShadow);

	glfwDestroyWindow(window);

	glfwTerminate();

	return 0;
}