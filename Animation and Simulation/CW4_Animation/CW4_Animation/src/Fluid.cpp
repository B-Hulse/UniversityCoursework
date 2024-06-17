#include "Fluid.h"
#include <iostream>
#include <stdlib.h>
#include <time.h>

#define NANC(a) if (isnan(a)) std::cout << "found nan" << std::endl

// Constructor
// w = the number of colums of particles
// h = number of rows of particles
// total number of particles is w*h
// The fluid should start in the canvas space coordinates 4,5 to 6,8
Fluid::Fluid(int w, int h) {
	srand(time(NULL));
	// Create all the particles in the fluid
	for (float y = 0; y < 3; y += (3. / (double)h))
		for (float x = 0; x < 2; x += (2. / (double)w))
		{
			// Random jitter for 
			glm::vec2 jit;
			jit.x = ((rand() % 101) / 1000.f) - 1.f;
			jit.y = ((rand() % 101) / 1000.f) - 1.f;
			particles.push_back(Particle(glm::vec2(XOFFSET + x, YOFFSET + y)));
			particles.back().pos += jit;
		}

	// Initialise the grid to be a spacial grid of width and height h
	// This assumes a canvas of 10x10
	gridW = ceil(10.f / H);
	grid.resize(gridW);
	for (size_t i = 0; i < gridW; i++)
	{
		grid[i].resize(gridW);
	}
}

void Fluid::update(Kernel fkern, bool visc) {
	// Clear the grid
	for (size_t i = 0; i < grid.size(); i++)
		for (size_t j = 0; j < grid.size(); j++)
			grid[i][j].clear();

	fillGrid();


	forceKern = fkern;
	visc_enabled = visc;
	for (size_t i = 0; i < particles.size(); i++)
	{
		calcDensity(i);
		particles[i].pressure = K * (particles[i].density - DENSITY_0);
	}
	for (size_t i = 0; i < particles.size(); i++)
	{
		updateParticle(i);
	}
}

void Fluid::fillGrid() {
	for (size_t i = 0; i < particles.size(); i++)
	{
		Particle* p = &(particles[i]);
		p->cGridX = floor(particles[i].pos.x / H);
		p->cGridY = floor(particles[i].pos.y / H);
		//std::cout << p->cGridX << "," << p->cGridY << std::endl;
		//std::cout << grid.size() << "," << grid[0].size() << std::endl;
		grid[p->cGridX][p->cGridY].push_back(p);
	}
}

void Fluid::calcDensity(size_t pID) {
	Particle* p = &(particles[pID]);
	float subTotal = 0;

	// New Grid System
	for (int xOff = -1; xOff <= 1; xOff++) {
		int gx = p->cGridX + xOff;
		if (gx < 0 or gx >= gridW) continue;
		for (int yOff = -1; yOff <= 1; yOff++) {
			int gy = p->cGridY + yOff;
			if (gy < 0 or gy >= gridW) continue;

			for (size_t i = 0; i < grid[gx][gy].size(); i++)
			{
				float rMag2 = glm::length2(p->pos - grid[gx][gy][i]->pos);
				subTotal += PMASS * w_poly6(rMag2);
			}
		}
	}

	p->density = subTotal;
}

void Fluid::updateForce(size_t pID) {
	Particle* p = &(particles[pID]);
	glm::vec2 subTotal = glm::vec2(0, 0);
	//for (size_t i = 0; i < particles.size(); i++)
	//{
	//	if (pID == i) continue;
	//	Particle* pi = &(particles[i]);

	//	glm::vec2 diff = pi->pos - p->pos;
	//	float rMag = glm::length(diff);
	//	glm::vec2 norm_diff;
	//	if (rMag == 0) {
	//		norm_diff = glm::normalize(glm::diskRand<float>(1));
	//	} else {
	//		norm_diff = diff/rMag;
	//	}

	//	float avgPressure = ((p->pressure + pi->pressure) / (2.f * pi->density));
	//	float fKernel=0;
	//	if (forceKern == Kernel::poly6) fKernel = w_gradpoly6(rMag);
	//	else if (forceKern == Kernel::Debrun) fKernel = w_graddebrun(rMag);

	//	glm::vec2 avgVel = ((pi->vel - p->vel) / (pi->density));
	//	float vKernel = w_lapvisc(rMag);

	//	// Calculate pressure force
	//	subTotal += -norm_diff * PMASS * avgPressure * fKernel;
	//	// Calculate viscocity force
	//	if (visc_enabled) subTotal += VISC * PMASS * avgVel * vKernel;
	//}

	for (int xOff = -1; xOff <= 1; xOff++) {
		int gx = p->cGridX + xOff;
		if (gx < 0 or gx >= gridW) continue;
		for (int yOff = -1; yOff <= 1; yOff++) {
			int gy = p->cGridY + yOff;
			if (gy < 0 or gy >= gridW) continue;

			for (size_t i = 0; i < grid[gx][gy].size(); i++)
			{
				Particle* pi = grid[gx][gy][i];
				if (p==pi) continue;

				glm::vec2 diff = pi->pos - p->pos;
				float rMag = glm::length(diff);
				glm::vec2 norm_diff;
				if (rMag == 0) {
					norm_diff = glm::normalize(glm::diskRand<float>(1));
				} else {
					norm_diff = diff/rMag;
				}

				float avgPressure = ((p->pressure + pi->pressure) / (2.f * pi->density));
				float fKernel=0;
				if (forceKern == Kernel::poly6) fKernel = w_gradpoly6(rMag);
				else if (forceKern == Kernel::Debrun) fKernel = w_graddebrun(rMag);

				glm::vec2 avgVel = ((pi->vel - p->vel) / (pi->density));
				float vKernel = w_lapvisc(rMag);
					
				// Calculate pressure force
				subTotal += -norm_diff * PMASS * avgPressure * fKernel;
				// Calculate viscocity force
				if (visc_enabled) subTotal += VISC * PMASS * avgVel * vKernel;

			}
		}
	}
	p->force += subTotal;
}

void Fluid::updateParticle(size_t pID) {
	Particle* p = &(particles[pID]);

	// Acceleration at time t
	glm::vec2 accel_t = p->accel;

	// Calculate acceleration at time t+1
	p->force = glm::vec2(0, 0);
	updateForce(pID);
	p->accel = p->force / PMASS;
	// Add gravity (added as accel as it is mass independent)
	p->accel += glm::vec2(0, -9.8f);

	// Calculate velocity at time t+1/2
	p->vel += accel_t * FRAMETIME * 0.5f;

	// Calculate position at time t+1
	p->pos += p->vel * FRAMETIME;

	// Calculate velocity at time t+1
	p->vel += p->accel * FRAMETIME * 0.5f;

	// Collision detection
	// Floor
	if (p->pos.y < (TBASE + PARTRAD)) {
		//newPos.y = (TBASE + PARTRAD);
		p->pos.y = (TBASE + PARTRAD) + ((rand() % 101) / 10000.f);
		p->vel.y *= -COR;
	}
	// Ceiling
	if (p->pos.y > (10-PARTRAD)) {
		//newPos.y = (TBASE + PARTRAD);
		p->pos.y = (10 - PARTRAD) - ((rand() % 101) / 10000.f);
		p->vel.y *= -COR;
	}
	
	// Left Wall
	if (p->pos.x < (TLEFT + PARTRAD)) {
		//newPos.x = (TLEFT + PARTRAD);
		p->pos.x = (TLEFT + PARTRAD) + ((rand() % 101) / 10000.f);
		p->vel.x *= -COR;
	}
	// Left Wall
	if (p->pos.x > (TRIGHT - PARTRAD)) {
		//newPos.x = (TRIGHT - PARTRAD);
		p->pos.x = (TRIGHT - PARTRAD) - ((rand() % 101) / 10000.f);
		p->vel.x *= -COR;
	}
}


void Fluid::render() {
	glPushMatrix();
	glColor4f(0.36, 0.68, 0.89,.5);

	Particle* p;
	for (size_t i = 0; i < particles.size(); i++)
	{
		p = &particles[i];
		glTranslatef(p->pos.x, p->pos.y, 0);
		renderParticle();
		glTranslatef(-p->pos.x, -p->pos.y, 0);
	}
	glPopMatrix();
}

// Draw particle as a circle around 0,0
void Fluid::renderParticle() {
	glBegin(GL_POLYGON);

	glVertex2f(PARTRAD, 0);
	glVertex2f(0.7 * PARTRAD, 0.7 * PARTRAD);
	glVertex2f(0, PARTRAD);
	glVertex2f(-0.7 * PARTRAD, 0.7 * PARTRAD);
	glVertex2f(-PARTRAD, 0);
	glVertex2f(-0.7 * PARTRAD, -0.7 * PARTRAD);
	glVertex2f(0, -PARTRAD);
	glVertex2f(0.7* PARTRAD, -0.7*PARTRAD);

	glEnd();
}

#pragma region Smoothing Functions
float Fluid::w_poly6(float rMag2) {
	if (rMag2 < H_2) {
		return P6_SCALE * pow(H_2 - rMag2, 3);
	}
	return 0;
}

float Fluid::w_gradpoly6(float rMag) {
	if (rMag <= H) {
		return GRADP6_SCALE * rMag * pow(H_2 - (rMag * rMag), 2);
	}
	return 0;
}

float Fluid::w_graddebrun(float rMag) {
	if (rMag <= H) {
		return DEB_SCALE * pow(H - rMag, 2);
	}
	return 0;
}

float Fluid::w_lapvisc(float rMag) {
	if (rMag <= H) {
		return VIS_SCALE * (H-rMag);
	}
	return 0;

}

#pragma endregion Smoothing function definitions (pol6, gradpol6, gradspiky)