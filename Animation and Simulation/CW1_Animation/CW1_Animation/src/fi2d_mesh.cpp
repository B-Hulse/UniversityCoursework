#include "fi2d_mesh.h"
#include <sstream>
#include <vector>
#include <iostream>


using namespace std;

int fi2d_mesh::init(string inStr) {
	stringstream fileContents(inStr);

	vector<string> tokens{ istream_iterator<string>{fileContents}, istream_iterator<string>{} };

	vCount = stoi(tokens[0]);
	fCount = stoi(tokens[1]);

	// Fill the verts vector with the vertices in the file
	for (size_t i = 0; i < vCount; i++)
	{
		float x = stof(tokens[2 + (i * 2)]);
		float y = stof(tokens[2 + (i * 2) + 1]);
		verts.push_back(Vert2f{ x,y });
	}

	// Fill the faces vector with the faces in the file
	for (size_t i = 0; i < fCount; i ++)
	{
		int v1 = stoi(tokens[2 + (vCount * 2) + (i * 3)]);
		int v2 = stoi(tokens[2 + (vCount * 2) + (i * 3) + 1]);
		int v3 = stoi(tokens[2 + (vCount * 2) + (i * 3) + 2]);
		faces.push_back(Face3i{ v1,v2,v3 });
	}

	scaleDown();

	return 1;
}

string fi2d_mesh::to2FI() {
	scaleUp();
	stringstream outStr;

	outStr << vCount << endl << fCount << endl;
	for (size_t i = 0; i < vCount; i++) {
		outStr << verts[i].x << " " << verts[i].y << endl;
	}
	for (size_t i = 0; i < fCount; i++) {
		outStr << faces[i].v1 << " " << faces[i].v2 << " " << faces[i].v3 << endl;
	}
	return outStr.str();
}

// Scale the mesh so that all verts are in the space of -1 to 1 in both directions
void fi2d_mesh::scaleDown() {
	float biggest = 0;
	for (size_t i = 0; i < vCount; i++)
	{
		if (abs(verts[i].x) > abs(biggest))
			biggest = verts[i].x;

		if (abs(verts[i].y) > abs(biggest))
			biggest = verts[i].y;
	}

	if (biggest > 1.f) {
		scale = 1.f / biggest;
		for (size_t i = 0; i < vCount; i++)
		{
			verts[i].x *= scale;
			verts[i].y *= scale;
		}
	}
}

// Return the mesh to it's original size
void fi2d_mesh::scaleUp() {
	for (size_t i = 0; i < vCount; i++)
	{
		verts[i].x /= scale;
		verts[i].y /= scale;
	}
}

fi2d_mesh::fi2d_mesh() {
	vCount = 0; 
	fCount = 0; 
	scale = 1.f;
}