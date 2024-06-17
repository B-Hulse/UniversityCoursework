#pragma once
#define GLEW_STATIC

#include "GL/glew.h"
#include "GLFW/glfw3.h"
#include "vendors/imgui/imgui.h"
#include "vendors/imgui/imgui_impl_opengl3.h"
#include "vendors/imgui/imgui_impl_glfw.h"
#include "Fluid.h"

#include <string>
#include <map>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>


using namespace std;

class FluidSimulator {
private:
	// Window Properties
	GLFWwindow* window;
	GLint WINWIDTH = 750, WINHEIGHT = 750;
	int BUFWIDTH, BUFHEIGHT;
	GLdouble fov = 90;

	// Camera Properties
	glm::vec3 camPos;
	glm::vec3 camDir;
	float camSpeed = 0.01f;
	float moveSpeed = 0.1f;

	// Program State
	int frameNo;
	double desiredFrameTime; // Desired framerate
	double currFrameTime;
	double prevFrameTime;
	double deltaFrameTime;
	bool playing = false;
	bool frameStep = false;
	bool autoTarget = false;

	// ImgGui variables
	bool showInfo = true, showControls = true;
	bool recording = false;

	// Fluid Variables
	Fluid *fluid;
	map<Fluid::Kernel, const char*> kernelStr = { {Fluid::Kernel::poly6,"poly6"},{Fluid::Kernel::Debrun,"Debrun"} };
	Fluid::Kernel pressureKernel = Fluid::Kernel::poly6;
	bool viscosity = false;
	bool surfaceTension = false;
	int resetSize[2] = { 40,80 };

	// Initialisation Functions
	int init();
	void mainLoop();

	// Input Functions
	void keyboardCheck(); 
	string validateName(string filename);

	// ImGui functions
	void imGuiDisplay();
	void showMenuBar();
	void showInfoGUI();
	void showControlsGUI();

	// Misc Funcs
	void savePNG();

	// Cleanup
	void cleanup();

public:
	FluidSimulator();
	void run();
	GLFWwindow* getWindow() { return window; };
	void mouseCallback(int button, int action, int mods);
};