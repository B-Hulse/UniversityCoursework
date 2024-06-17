#include "Cloth.h"
#include <GLFW/glfw3.h>

#include <iostream>

void Cloth::getMassAndSprings(OBJ* o) {
	points.clear();
	springs.clear();

	// Get all the points in the object
	for (size_t i = 0; i < o->verts.size(); i++)
	{
		Vert v = o->verts[i];
		MassPoint p(glm::vec3(v.x, v.y, v.z));
		points.push_back(p);
		norms.push_back(glm::vec3(0., 1., 0.));
	}
	vPerSide = sqrt(points.size());

	// Create a 2D bool map
	vector<vector<bool>> joined;
	joined.resize(o->verts.size());
	for (size_t i = 0; i < o->verts.size(); i++)
	{
		joined[i].assign(o->verts.size(), false);
	}

	// Get all springs
	// Dont repeat springs between verts with multiple edges
	for (size_t i = 0; i < o->faces.size(); i++)
	{
		Face f = o->faces[i];

		size_t s = f.v.size();
		for (size_t j = 0; j < s; j++)
		{
			if (!(joined[f.v[j]][f.v[(j + 1) % s]])) {
				Spring sp(f.v[j], f.v[(j + 1) % s]);
				springs.push_back(sp);

				joined[f.v[j]][f.v[(j + 1) % s]] = true;
				joined[f.v[(j + 1) % s]][f.v[j]] = true;
			}
		}
	}

	faces = o->faces;

	// Set all spring lengths
	for (size_t i = 0; i < springs.size(); i++)
	{
		Spring s = springs[i];

		glm::vec3 diff = points[s.m1].pos - points[s.m2].pos;
		s.Lr = glm::length(diff);
		s.Lc = glm::length(diff);
		springs[i] = s;
	}

	c1OG = points[0].pos;
	c2OG = points[points.size() - 1].pos;
}

void Cloth::render(bool wireframe) {
	float l[4] = { .7f, .7f ,.7f,1.f };
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, l);
	glPushMatrix();

	// Draw surface
	glColor3f(0.5, 0.5, 0.5);
	glBegin(GL_TRIANGLES);
	for (size_t i = 0; i < faces.size(); i++) {
		Face f = faces[i];
		for (size_t j = 0; j < f.v.size(); j++)
		{
			MassPoint v = points[f.v[j]];
			glm::vec3 n = norms[f.v[j]];
			glNormal3f(n.x, n.y, n.z);
			glVertex3f(v.pos.x, v.pos.y, v.pos.z);
		}
	}
	glEnd();

	// Draw wireframe
	if (wireframe) {
		glLineWidth(3.);
		glColor3f(1., 0., 0.);
		glBegin(GL_LINES);
		for (size_t i = 0; i < springs.size(); i++)
		{
			glm::vec3 *p1, *p2;
			p1 = &(points[springs[i].m1].pos);
			p2 = &(points[springs[i].m2].pos);

			glVertex3f(p1->x, p1->y, p1->z);
			glVertex3f(p2->x, p2->y, p2->z);
		}
		glEnd();
	}

	glPopMatrix();
}

void Cloth::updatePhys(bool sphere, bool floor, bool pinCorners, bool sphereSpin, float sphereSpeed, bool wind) {
	// Test Code
	if (pinCorners) {
		points[0].pos = c1OG;
		points[points.size()-1].pos = c2OG;
	}

	for (size_t i = 0; i < springs.size(); i++)
	{
		Spring* s = &(springs[i]);

		glm::vec3 *p1, *p2;
		p1 = &(points[s->m1].pos);
		p2 = &(points[s->m2].pos);

		s->Lc = glm::length(*p2 - *p1);

		float weight = s->Ks * (s->Lc - s->Lr);

		glm::vec3 force = weight * ((*p2 - *p1)/s->Lc);

		points[s->m1].force += .1f * force;
		points[s->m2].force -= .1f * force;
	}

	for (size_t i = 0; i <points.size(); i++)
	{
		if (pinCorners and (i == 0 or i == points.size() - 1)) continue;
		MassPoint* p = &(points[i]);

		// Gravity
		p->force += glm::vec3(0.f, -0.0001f, 0.f);

		// Wind
		if (wind) p->force += glm::vec3(0.0002f, 0.f, 0.f);

		p->acc = p->force;
		p->force = glm::vec3(0., 0., 0.);
		p->vel *= 0.9;
		p->vel += p->acc;
		p->pos += p->vel;

		if (sphere) {
			// Find the distance form sphere center to the point
			glm::vec3 cCenter = glm::vec3(0., 0.5, 0.);
			glm::vec3 centToP = p->pos - cCenter;
			float cRad = 0.5f;
			float distToP = glm::length(centToP);
			if (distToP <= cRad) {
				p->pos = cCenter + (cRad * (centToP/distToP));
				// Apply sphere friction
				if (sphereSpin) {
					float H = cRad - abs(centToP.y);
					if (H > 0.01 and H <= cRad) {
						float surfaceMag = sphereSpeed * sqrt(H * (2.*cRad - H));
						glm::vec3 surfaceVel = glm::vec3(-centToP.z, 0., centToP.x);
						surfaceVel = surfaceMag * glm::normalize(surfaceVel);
						//cout << surfaceVel.x << "," << surfaceVel.y << "," << surfaceVel.z << endl;
						p->force += 0.001f * surfaceVel;
					}
				}
			}
		}

		if (floor) {
			// Check floor boundary
			if (p->pos.y < 0.01f) {
				p->pos.y = 0.01f;
			}
		}

	}
}

void Cloth::updateNorms() {
	glm::vec3 HVec, VVec, NNorm;
	for (size_t i = 0; i < points.size(); i++)
	{
		int HVeci1 = (i % vPerSide) == 0 ? i : i - 1;
		int HVeci2 = (i % vPerSide) == vPerSide - 1 ? i : i + 1;
		HVec = points[HVeci2].pos - points[HVeci1].pos;

		int VVeci1 = (i < vPerSide)? i : i - vPerSide;
		int VVeci2 = (i >= vPerSide*(vPerSide-1))? i : i + vPerSide;
		VVec = points[VVeci2].pos - points[VVeci1].pos;

		NNorm = glm::normalize(glm::cross(HVec, VVec));

		//cout << "H " << HVeci1 << " " << HVeci2 << " V " << VVeci1 << " " << VVeci2 << endl;
		//cout << VVec.x << "," << VVec.y << "," << VVec.z << " X " << HVec.x << "," << HVec.y << "," << HVec.z << " = " << NNorm.x << "," << NNorm.y << "," << NNorm.z << endl;
		if (glm::dot(NNorm, glm::vec3(norms[i].x, norms[i].y, norms[i].z)) > 0) {
			norms[i] = NNorm;
		}
		else {
			norms[i] = -1.f * NNorm;
		}
	}
}