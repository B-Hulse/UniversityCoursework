#include "FluidSimulator.h"
#include <sstream>
#include <iostream>
#include <fstream>
#include <string>


void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
}

int main() {
	FluidSimulator s;
	glfwSetMouseButtonCallback(s.getWindow(), mouse_button_callback);
	s.run();

	return 1;
}

