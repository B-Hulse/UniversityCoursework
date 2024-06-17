#pragma once
#include <vector>

using namespace std;

class BMatrix {
public:
	// Column major format of matrix
	// address as (x,y) is index x*height + y
	vector<vector<double>> data;
	int width, height;
public:
	BMatrix();
	BMatrix(int w, int h);
	~BMatrix();
	void resize(int w, int h);
	void zero();
	BMatrix transpose();
	BMatrix psuedoInv();
	//BMatrix inverse();
	BMatrix operator*(const BMatrix& other);

	//void operator =(const Jacobian& other);
	// Jacobian operator*(Vector& other) // Operatore to multiply by E = posg-pos
};