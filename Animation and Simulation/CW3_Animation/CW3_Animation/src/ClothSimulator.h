#pragma once
#define GLEW_STATIC

#include "GL/glew.h"
#include "GLFW/glfw3.h"
#include "vendors/imgui/imgui.h"
#include "vendors/imgui/imgui_impl_opengl3.h"
#include "vendors/imgui/imgui_impl_glfw.h"
#include "OBJ.h"
#include "Cloth.h"

#include <string>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>


using namespace std;

class ClothSimulator {
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
	bool autoTarget = false;

	// ImgGui variables
	bool showInfo = true, showControls = false;
	bool showSaveDialog = false, showLoadDialog = false;
	bool showMakeWindow = false;
	char fnbuf[256];
	bool showFNFModal = false;
	bool showSaveErrorModal = false;
	string saveErrorContents = "Error Saving File";
	float makeCWidth = 2;
	int makeCDivs = 10;
	float makeCHeight = 2.5;
	bool recording = false;
	bool lighting = false;

	// OBJ instance
	OBJ* object;
	Cloth* cloth;

	// Cloth Control
	bool clothWireframe = false;
	bool sphereActive = false;
	bool sphereSpin = false;
	bool floorActive = true;
	bool cornersPin = false;
	bool wind = false;


	float sphereSpinSpeed = 1;

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
	void showFileDialog();
	void showMakeOBJ();

	// Misc Funcs
	bool loadCloth(string filename);
	void makeCloth();
	void savePNG();

	// Cleanup
	void cleanup();


public:
	ClothSimulator();
	void run();
	GLFWwindow* getWindow() { return window; };
	void mouseCallback(int button, int action, int mods);
};