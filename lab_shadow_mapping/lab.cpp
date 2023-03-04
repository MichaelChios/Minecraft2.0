// Include C++ headers
#include <iostream>
#include <string>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <glfw3.h>

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Shader loading utilities and other
#include <common/shader.h>
#include <common/util.h>
#include <common/camera.h>
#include <common/light.h> 
#include <common/NoiseGenerator.h>
#include <common/Chunk.h>
#include <common/FastNoiseLite.h>


using namespace std;
using namespace glm;

// Function prototypes
void initialize();
void createContext();
void mainLoop();
void free();

#define W_WIDTH 1600
#define W_HEIGHT 790
#define TITLE "Minecraft_2.0"

#define SHADOW_WIDTH 1024
#define SHADOW_HEIGHT 1024

// Creating a structure to store the material parameters of an object
struct Material {
	vec4 Ka;
	vec4 Kd;
	vec4 Ks;
	float Ns;
};

// Global Variables
GLFWwindow* window;
Camera* camera;
Light* light;
GLuint shaderProgram, depthProgram, miniMapProgram;
GLuint modelDiffuseTexture, modelSpecularTexture;
GLuint depthFrameBuffer, depthTexture;
NoiseGenerator ng = NoiseGenerator(1000);
FastNoiseLite noise;

// Locations for shaderProgram
GLuint viewMatrixLocation;
GLuint projectionMatrixLocation;
GLuint modelMatrixLocation;
GLuint KaLocation, KdLocation, KsLocation, NsLocation;
GLuint LaLocation, LdLocation, LsLocation;
GLuint lightPositionLocation;
GLuint lightPowerLocation;
GLuint diffuseColorSampler;
GLuint specularColorSampler;
GLuint useTextureLocation;
GLuint depthMapSampler;
GLuint lightVPLocation;
GLuint lightDirectionLocation;
GLuint lightFarPlaneLocation;
GLuint lightNearPlaneLocation;

// Locations for depthProgram
GLuint shadowViewProjectionLocation;
GLuint shadowModelLocation;

GLuint grassTexture, snowTexture, dirtTexture, stoneTexture, waterTexture;
Drawable* model1;

// Creating a function to upload (make uniform) the light parameters to the shader program
void uploadLight(const Light& light) {
	glUniform4f(LaLocation, light.La.r, light.La.g, light.La.b, light.La.a);
	glUniform4f(LdLocation, light.Ld.r, light.Ld.g, light.Ld.b, light.Ld.a);
	glUniform4f(LsLocation, light.Ls.r, light.Ls.g, light.Ls.b, light.Ls.a);
	glUniform3f(lightPositionLocation, light.lightPosition_worldspace.x,
		light.lightPosition_worldspace.y, light.lightPosition_worldspace.z);
	glUniform1f(lightPowerLocation, light.power);
}

// Creating a function to upload the material parameters of a model to the shader program
void uploadMaterial(const Material& mtl) {
	glUniform4f(KaLocation, mtl.Ka.r, mtl.Ka.g, mtl.Ka.b, mtl.Ka.a);
	glUniform4f(KdLocation, mtl.Kd.r, mtl.Kd.g, mtl.Kd.b, mtl.Kd.a);
	glUniform4f(KsLocation, mtl.Ks.r, mtl.Ks.g, mtl.Ks.b, mtl.Ks.a);
	glUniform1f(NsLocation, mtl.Ns);
}

void createContext() {

	// Create and complile our GLSL program from the shader
	shaderProgram = loadShaders("ShadowMapping.vertexshader", "ShadowMapping.fragmentshader");

	depthProgram = loadShaders("Depth.vertexshader", "Depth.fragmentshader");

	miniMapProgram = loadShaders("SimpleTexture.vertexshader", "SimpleTexture.fragmentshader");

	// Get pointers to uniforms
	// --- shaderProgram ---
	projectionMatrixLocation = glGetUniformLocation(shaderProgram, "P");
	viewMatrixLocation = glGetUniformLocation(shaderProgram, "V");
	modelMatrixLocation = glGetUniformLocation(shaderProgram, "M");

	// For phong lighting
	KaLocation = glGetUniformLocation(shaderProgram, "mtl.Ka");
	KdLocation = glGetUniformLocation(shaderProgram, "mtl.Kd");
	KsLocation = glGetUniformLocation(shaderProgram, "mtl.Ks");
	NsLocation = glGetUniformLocation(shaderProgram, "mtl.Ns");
	LaLocation = glGetUniformLocation(shaderProgram, "light.La");
	LdLocation = glGetUniformLocation(shaderProgram, "light.Ld");
	LsLocation = glGetUniformLocation(shaderProgram, "light.Ls");
	lightPositionLocation = glGetUniformLocation(shaderProgram, "light.lightPosition_worldspace");
	lightPowerLocation = glGetUniformLocation(shaderProgram, "light.power");
	diffuseColorSampler = glGetUniformLocation(shaderProgram, "diffuseColorSampler");
	specularColorSampler = glGetUniformLocation(shaderProgram, "specularColorSampler");

	useTextureLocation = glGetUniformLocation(shaderProgram, "useTexture");

	// Locations for shadow rendering
	depthMapSampler = glGetUniformLocation(shaderProgram, "shadowMapSampler");
	lightVPLocation = glGetUniformLocation(shaderProgram, "lightVP");

	// --- depthProgram ---
	shadowViewProjectionLocation = glGetUniformLocation(depthProgram, "VP");
	shadowModelLocation = glGetUniformLocation(depthProgram, "M");

	// Tell opengl to generate a framebuffer
	glGenFramebuffers(1, &depthFrameBuffer);
	// Binding the framebuffer, all changes bellow will affect the binded framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, depthFrameBuffer);

	glGenTextures(1, &depthTexture);
	glBindTexture(GL_TEXTURE_2D, depthTexture);

	// Telling opengl the required information about the texture
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0,
		GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);

	// Since the depth buffer is only for the generation of the depth texture, 
	// there is no need to have a color output
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	// Finally, we have to always check that our frame buffer is ok
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		glfwTerminate();
		throw runtime_error("Frame buffer not initialized correctly");
	}

	// Binding the default framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void free() {
	// Delete Shader Programs
	glDeleteProgram(shaderProgram);
	glDeleteProgram(depthProgram);
	glDeleteProgram(miniMapProgram);

	glfwTerminate();
}


void depth_pass(mat4 viewMatrix, mat4 projectionMatrix) {

	// Setting viewport to shadow map size
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);

	// Binding the depth framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, depthFrameBuffer);

	// Cleaning the framebuffer depth information (stored from the last render)
	glClear(GL_DEPTH_BUFFER_BIT);

	// Selecting the new shader program that will output the depth component
	glUseProgram(depthProgram);

	// sending the view and projection matrix to the shader
	mat4 view_projection = projectionMatrix * viewMatrix;
	glUniformMatrix4fv(shadowViewProjectionLocation, 1, GL_FALSE, &view_projection[0][0]);

	// binding the default framebuffer again
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

vector<Chunk*> chunkMatrix = {};

void lighting_pass(mat4 viewMatrix, mat4 projectionMatrix) {

	// Binding a frame buffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, W_WIDTH, W_HEIGHT);

	// Clearing color and depth info
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Selecting shader program
	glUseProgram(shaderProgram);

	// Uploading the light parameters to the shader program
	uploadLight(*light);

	// Sending the shadow texture to the shaderProgram
	glActiveTexture(GL_TEXTURE0);								// Activate texture position
	glBindTexture(GL_TEXTURE_2D, depthTexture);			// Assign texture to position 
	glUniform1i(depthMapSampler, 0);

	// Sending the light View-Projection matrix to the shader program
	mat4 light_VP = light->lightVP();
	glUniformMatrix4fv(lightVPLocation, 1, GL_FALSE, &light_VP[0][0]);

	// --------------------- Drawing scene objects --------------------- //
	vec3 trans = vec3(0, 0, 0);
	glUniformMatrix4fv(viewMatrixLocation, 1, GL_FALSE, &viewMatrix[0][0]);
	glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE, &projectionMatrix[0][0]);

	for (int i = 0; i < chunkMatrix.size(); i++) {
		chunkMatrix[i]->draw(modelMatrixLocation, useTextureLocation, model1, grassTexture, snowTexture, dirtTexture, stoneTexture, waterTexture);
	}

	// Creating a model matrix
	mat4 modelMatrix = translate(mat4(), vec3(0.0, 0.0, -5.0));
	glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &modelMatrix[0][0]);

	// Setting up texture to display on shader program          //  --- Texture Pipeline ---
	glActiveTexture(GL_TEXTURE1);								// Activate texture position
	glBindTexture(GL_TEXTURE_2D, modelDiffuseTexture);			// Assign texture to position 
	glUniform1i(diffuseColorSampler, 1);						// Assign sampler to that position

	glActiveTexture(GL_TEXTURE2);								//
	glBindTexture(GL_TEXTURE_2D, modelSpecularTexture);			// Same process for specular texture
	glUniform1i(specularColorSampler, 2);						//

	glUniform1i(useTextureLocation, 1);

}

int ftime = 0;
float start = (float)glfwGetTime();


void mainLoop() {

	light->update();
	mat4 light_proj = light->projectionMatrix;
	mat4 light_view = light->viewMatrix;

	model1 = new Drawable("Grass_Block.obj");
	grassTexture = loadSOIL("Grass_Block_TEX.jpg");
	waterTexture = loadSOIL("Water.png");
	snowTexture = loadSOIL("snow.jpg");
	dirtTexture = loadSOIL("dirt.png");
	stoneTexture = loadSOIL("minecraft_stone.png");

	for (int i = 0; i < 20; i++) {
		for (int j = 0;j < 20;j++) {
			Chunk* c = new Chunk(i, j);
			c->generateSea();
			c->removeInnerCubes();
			chunkMatrix.push_back(c);
		}
	}

	do {
		light->update();
		mat4 light_proj = light->projectionMatrix;
		mat4 light_view = light->viewMatrix;

		// Create the depth buffer
		depth_pass(light_view, light_proj);

		// Getting camera information
		camera->update();
		mat4 projectionMatrix = camera->projectionMatrix;
		mat4 viewMatrix = camera->viewMatrix;

		// Rendering the scene from light's perspective when F1 is pressed
		if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS) {
			lighting_pass(light_view, light_proj);
		}
		else {
			// Render the scene from camera's perspective
			lighting_pass(viewMatrix, projectionMatrix);
		}

		ftime += 1;
		if (ftime == 500) {
			cout << "Fps: " << ftime / (glfwGetTime() - start) << endl;
			ftime = 0;
			start = (float)glfwGetTime();
		}

		glfwSwapBuffers(window);
		glfwPollEvents();
	} while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
		glfwWindowShouldClose(window) == 0);
}


void initialize() {
	// Initialize GLFW
	if (!glfwInit()) {
		throw runtime_error("Failed to initialize GLFW\n");
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow(W_WIDTH, W_HEIGHT, TITLE, NULL, NULL);
	if (window == NULL) {
		glfwTerminate();
		throw runtime_error(string(string("Failed to open GLFW window.") +
			" If you have an Intel GPU, they are not 3.3 compatible." +
			"Try the 2.1 version.\n"));
	}
	glfwMakeContextCurrent(window);

	// Start GLEW extension handler
	glewExperimental = GL_TRUE;

	// Initialize GLEW
	if (glewInit() != GLEW_OK) {
		glfwTerminate();
		throw runtime_error("Failed to initialize GLEW\n");
	}

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

	// Hide the mouse and enable unlimited movement
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Set the mouse at the center of the screen
	glfwPollEvents();
	glfwSetCursorPos(window, W_WIDTH / 2, W_HEIGHT / 2);

	// Sky background color
	glClearColor(0.53f, 0.81f, 0.92f, 1.0f);

	// Enable depth test
	glEnable(GL_DEPTH_TEST);

	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);

	// enable texturing and bind the depth texture
	glEnable(GL_TEXTURE_2D);

	// Log
	logGLParameters();

	// Create camera
	camera = new Camera(window);

	// Creating a custom light 
	light = new Light(window,
		vec4{ 1, 1, 1, 1 },
		vec4{ 1, 1, 1, 1 },
		vec4{ 1, 1, 1, 1 },
		vec3{ 51, 120, 110 },
		135.0f
	);

}

int main(void) {
	try {
		initialize();
		createContext();
		mainLoop();
		free();
	}
	catch (exception& ex) {
		cout << ex.what() << endl;
		getchar();
		free();
		return -1;
	}

	return 0;
}