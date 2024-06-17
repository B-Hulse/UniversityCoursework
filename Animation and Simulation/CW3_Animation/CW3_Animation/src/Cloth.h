#pragma once
#include <vector>
#include <glm/glm.hpp>

#include "OBJ.h"

using namespace std;

class Cloth {

	struct MassPoint {
		glm::vec3 pos;
		glm::vec3 vel = glm::vec3(0,0,0);
		glm::vec3 acc = glm::vec3(0, 0, 0);

		glm::vec3 force = glm::vec3(0, 0, 0);

		MassPoint(glm::vec3 v) {
			pos = v;
		}
	};

	struct Spring {
		// Indixes of the mass points that the spring attatches
		int m1, m2;
		float Lr=0.f;
		float Lc=0.f;
		float Ks=1.f;

		Spring(int _m1, int _m2) {
			m1 = _m1;
			m2 = _m2;
		}
	};
public:
	vector<MassPoint> points;
	vector<glm::vec3> norms; // Per Vertex normal
	vector<Spring> springs;
	vector<Face> faces;
	int vPerSide=0;
	glm::vec3 c1OG=glm::vec3(), c2OG = glm::vec3();

public:
	void getMassAndSprings(OBJ* obj);
	void render(bool wireframe);
	void updatePhys(bool sphere, bool floor, bool pinCorners, bool sphereSpin, float sphereSpeed, bool wind);
	void updateNorms();
};