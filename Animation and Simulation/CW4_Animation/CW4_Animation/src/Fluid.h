#pragma once
#include "GL/glew.h"
#include "GLFW/glfw3.h"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/random.hpp>
#include <glm/gtx/norm.hpp>
//#include <glm/vec2.hpp>
#include <vector>
using namespace std;

class Fluid {
private:
	// Structure for the particles in the fluid
	struct Particle {
		// Variables that are throughout the simulation
		glm::vec2 pos;
		// Since we use leapfrog, the vel is at timestep i+1/2
		glm::vec2 vel = glm::vec2(0.f, 0.0f);

		// Variables that are at each timestep
		glm::vec2 accel = glm::vec2(0.f, 0.f);
		glm::vec2 force = glm::vec2(0.f, 0.f);
		float density = 0.;
		float pressure = 0.;

		// The grid index of the particle
		int cGridX = 0, cGridY = 0;

		Particle(glm::vec2 _pos) {
			pos = _pos;
		}
	};

	// Configuaration Variables

	// Initial water position offsets
	const float XOFFSET = 4.;
	const float YOFFSET = 6.;

	// Particle data
	const float PARTRAD = .03;
	const float PMASS = 65.f;
	const float DENSITY_0 = 3000.;
	const float COR = .5f;
	const float VISC = 0.1f;

	// Boundary positions
	const float TBASE = 1.1f;
	const float TLEFT = 2.5f;
	const float TRIGHT = 7.5f;

	// Simulation data
	const float FRAMETIME = 0.008f;
	const float H = .075f; // Kernel Width
	const float K = .001f;
	const float H_2 = H * H;
	// Tension coeff
	const float T = 1.f;;

	// Kernel Calculations
	const float P6_SCALE = 4. / (glm::pi<float>() * pow(H, 8));
	const float GRADP6_SCALE = -24. / (glm::pi<float>() * pow(H, 8));
	//const float P6_SCALE = 315. / (64. * glm::pi<float>() * pow(H, 9));
	const float DEB_SCALE = -30.f / (glm::pi<float>() * pow(H, 5.f));
	const float VIS_SCALE = 40.f / (glm::pi<float>() * pow(H, 5.f));


	vector<Particle> particles;
	vector<vector<vector<Particle*>>> grid;
	int gridW;

	void fillGrid();

	// Smoothing funcitons
	float w_poly6(float rMag2);
	float w_gradpoly6(float rMag);
	float w_graddebrun(float rMag);
	float w_lapvisc(float rMag);

public:
	enum class Kernel { poly6, Debrun };
	Kernel forceKern = Kernel::poly6;
	bool visc_enabled = false;

	// Constructor
	Fluid(int w, int h);

	// Update functions
	void update(Kernel fkern, bool visc);
	void updateParticle(size_t pID);
	void updateForce(size_t pID);
	void calcDensity(size_t pID);


	// Rendering functions
	void render();
	void renderParticle();
};