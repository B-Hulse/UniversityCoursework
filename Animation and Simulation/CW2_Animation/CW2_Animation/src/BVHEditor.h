#pragma once
#include "GL/glew.h"
#include "GLFW/glfw3.h"
#include "vendors/imgui/imgui.h"
#include "vendors/imgui/imgui_impl_opengl3.h"
#include "vendors/imgui/imgui_impl_glfw.h"
//#include "vendors/BVH.h"
#include "BVH.h"

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

class BVHEditor {
private:
	// Window Properties
	GLFWwindow* window;
	GLint WINWIDTH = 750, WINHEIGHT = 750;
	int BUFWIDTH, BUFHEIGHT;
	BVH* currBVH;
	GLdouble fov = 90;

	// Camera Properties
	glm::vec3 camPos;
	glm::vec3 camDir;
	float camSpeed = 0.01f;
	float moveSpeed = 0.1f;

	// Selecting attributes
	glm::vec3 crVec = glm::vec3(0, 0, 1);
	glm::vec3 crOri = glm::vec3(0, 0, 0);
	int selectedJoint = -1;
	glm::vec3 ikTarget = glm::vec3(0., 0., 0.);

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
	char fnbuf[256];
	bool showFNFModal = false;
	bool showSaveErrorModal = false;
	string saveErrorContents = "Error Saving File";
	float targetP[3] = { 0.,0.,0. };

	// Initialisation Functions
	int init();
	void mainLoop();
	bool loadBVH(string filename);

	// Input Functions
	void keyboardCheck();

	// ImGui functions
	void imGuiDisplay();
	void showMenuBar();
	void showInfoGUI();
	void showControlsGUI();
	void showFileDialog();

	// BVH File saving function
	bool writeBVHFile(string filename);
	string validateName(string filename);
	void writeJoint(BVH::Joint* j, ofstream* file);
	void writeMotion(ofstream* file);

	// Cleanup
	void cleanup();

	// Check if the point p is on the line parameterised by o + tv
	float distanceToClick(glm::vec3 p);
	void selectJoint();

public:
	BVHEditor();
	void run();
	GLFWwindow* getWindow() {return window;};
	void mouseCallback(int button, int action, int mods);


};