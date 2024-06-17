#include "ClothSimulator.h"
#include <sstream>
#include <iostream>
#include <fstream>
#include <string>


void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
}

// Function to make a cloth obj file
void makeOBJ() {
	ofstream f("files/cloth_test.obj");

	float vC = 21;

	for (size_t i = 0; i < vC; i++)
		for (size_t j = 0; j < vC; j++)
		{
			float x, y = 2.5, z;
			x = (i * 2.f / (vC - 1.f)) - 1.f;
			z = (j * 2.f / (vC - 1.f)) - 1.f;
			f << "v " << x << " " << y << " " << z << std::endl;
		}

	for (size_t i = 1; i <= (vC * vC)-vC ; i++) {
		if (i % (int)vC == 0) continue;
		int j1 = i + vC;
		f << "f " << i << " " << j1 << " " << i + 1 << std::endl;
		f << "f " << i + 1 << " " << j1 << " " << j1 + 1 << std::endl;
	}
	f.close();
}

int main() {
	makeOBJ();
	ClothSimulator s;
	glfwSetMouseButtonCallback(s.getWindow(), mouse_button_callback);
	s.run();
	

	return 1;
}

