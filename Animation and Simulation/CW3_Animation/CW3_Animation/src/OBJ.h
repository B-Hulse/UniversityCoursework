#pragma once
#include <string>
#include <vector>

using namespace std;

struct Vert {
	float x,y,z,w=1.;
};

struct Face {
	vector<int> v;
	vector<int> n;
};

class OBJ {
public:
	OBJ();
	bool loadOBJ(string filename);
	bool saveOBJ(string filename);
	void render();

	vector<string> fileData;
	vector<Vert> verts;
	vector<Vert> norms;
	vector<Face> faces;

	bool parseFile();
};