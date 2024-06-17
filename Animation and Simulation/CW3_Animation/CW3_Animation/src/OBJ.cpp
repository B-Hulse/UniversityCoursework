#include "OBJ.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

//#include <GL/glew.h>
#include <GLFW/glfw3.h>

OBJ::OBJ() {

}

// Read file contents to the fileData vector, line by line
// return the success state of the object loading
bool OBJ::loadOBJ(string filename) {
	ifstream f(filename.c_str());

	if (!f.is_open()) {
		cout << "File could not be opened" << endl;
		return false;
	}

	string line;
	while (getline(f,line)) {
		fileData.push_back(line);
	}

	if (fileData.size() == 0) {
		cout << "File is empty" << endl;
		f.close();
		return false;
	}

	f.close();

	return parseFile();
}

// Parse the data in the fileData vector
// ret: the success state of parsing the data
bool OBJ::parseFile() {
	// track the line being parsed
	size_t cl = 0;

	string token;
	stringstream currLine;
	while (cl < fileData.size()) {
		currLine = stringstream(fileData[cl++]);

		vector<string> tokens;
		while (currLine >> token) tokens.push_back(token);

		if (tokens.size() == 0) continue;

		if (tokens[0] == "v") {
			Vert newV;

			newV.x = stof(tokens[1]);
			newV.y = stof(tokens[2]);
			newV.z = stof(tokens[3]);
			newV.w = tokens.size() > 4 ? stof(tokens[4]) : 1.f;

			verts.push_back(newV);
		}
		else if (tokens[0] == "vn") {
			Vert newN;

			newN.x = stof(tokens[1]);
			newN.y = stof(tokens[2]);
			newN.z = stof(tokens[3]);

			norms.push_back(newN);
		}
		else if (tokens[0] == "f") {
			Face newF;

			// How many parameters the face has
			int pCount = count(tokens[1].begin(), tokens[1].end(), '/') + 1;

			for (size_t i = 1; i < tokens.size(); i++)
			{
				newF.v.push_back(stoi(tokens[i])-1);
				if (pCount == 3) {
					size_t found = tokens[i].find_last_of('/');
					newF.n.push_back(stoi(tokens[i].substr(found+1))-1);
				}
			}

			faces.push_back(newF);
		}
	}
	// Generate the normals of the verts if not set
	for (size_t i = 0; i < faces.size(); i++)
	{
		Face f = faces[i];
		if (f.n.size() != 0) continue;
		Vert U, V, N;
		U.x = verts[f.v[1]].x - verts[f.v[0]].x;
		U.y = verts[f.v[1]].y - verts[f.v[0]].y;
		U.x = verts[f.v[1]].z - verts[f.v[0]].z;
		V.x = verts[f.v[2]].x - verts[f.v[0]].x;
		V.y = verts[f.v[2]].y - verts[f.v[0]].y;
		V.x = verts[f.v[2]].z - verts[f.v[0]].z;
		N.x = (U.y * V.z) - (U.z * V.y);
		N.y = (U.z * V.x) - (U.x * V.z);
		N.z = (U.x * V.y) - (U.y * V.x);
		float nSize = sqrt(N.x * N.x + N.y * N.y + N.z * N.z);
		int newNI = norms.size();
		N.x /= nSize; N.y /= nSize; N.z /= nSize;
		norms.push_back(N);
		for (size_t j = 0; j < f.v.size(); j++)
		{
			faces[i].n.push_back(newNI);
		}
	}
	return true;
}

bool OBJ::saveOBJ(string filename) {
	ofstream f(filename.c_str());

	if (!f.is_open()) {
		cout << "File could not be opened" << endl;
		return false;
	}

	for (size_t i = 0; i < verts.size(); i++)
	{
		Vert v = verts[i];
		f << "v " << v.x << " " << v.y << " " << v.z << " " << v.w << endl;
	}

	for (size_t i = 0; i < norms.size(); i++)
	{
		Vert n = norms[i];
		f << "vn " << n.x << " " << n.y << " " << n.z << endl;
	}

	for (size_t i = 0; i < faces.size(); i++)
	{
		Face face = faces[i];
		f << "f";
		for (size_t j = 0; j < face.v.size(); j++)
		{
			f << " " << face.v[j] + 1;
			if (face.n.size() != 0) {
				f << "//" << face.n[j] + 1;
			}
		}
		f << endl;
	}

	f.close();

	return true;
}

void OBJ::render() {
	glPushMatrix();

	for (size_t i = 0; i < faces.size(); i++) {
		Face f = faces[i];
		glBegin(GL_POLYGON);
		for (size_t j = 0; j < f.v.size(); j++)
		{
			Vert v = verts[f.v[j]];
			if (f.n.size() != 0) {
				Vert n = norms[f.n[j]];
				glNormal3f(n.x, n.y, n.z);
			}
			glVertex3f(v.x, v.y, v.z);
		}
		glEnd();
	}

	glPopMatrix();
}