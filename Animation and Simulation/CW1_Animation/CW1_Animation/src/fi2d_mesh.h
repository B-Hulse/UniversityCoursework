#pragma once
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>

using namespace std;

// A structure holding the x and y coordinates of a vertex
struct Vert2f { float x, y; };
// A structure holding the indices of the 3 vertexes that belong to a face
struct Face3i { int v1, v2, v3; };

class fi2d_mesh
{
public:
	vector<Vert2f> verts;
	vector<Face3i> faces;
	unsigned int vCount;
	unsigned int fCount;
	float scale;

	fi2d_mesh();
	int init(std::string);
	string to2FI();
private:
	void scaleDown();
	void scaleUp();
};