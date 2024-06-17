#include "BVHEditor.h"
#include <sstream>
#include <iostream>
#include <string>

BVHEditor bvhEditor;

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	bvhEditor.mouseCallback(button, action, mods);
}

int main() {
	glfwSetMouseButtonCallback(bvhEditor.getWindow(), mouse_button_callback);
	bvhEditor.run();
	return 1;
}