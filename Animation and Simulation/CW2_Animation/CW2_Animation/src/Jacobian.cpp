#include "Jacobian.h"
#include <iostream>
#include <Eigen/Core>

BMatrix::BMatrix() {
	width = 0;
	height = 0;
}

BMatrix::BMatrix(int w, int h) : BMatrix() {
	resize(w, h);
	zero();
}

BMatrix::~BMatrix() {
}

void BMatrix::resize(int w, int h) {
	data.resize(h);
	for (size_t i = 0; i < h; i++)
	{
		data[i].resize(w);
	}
	width = w;
	height = h;
}

void BMatrix::zero() {
	for (size_t i = 0; i < height; i++)
		for (size_t j = 0; j < width; j++)
		{
			data[i][j] = 0;
		}
}

//BMatrix BMatrix::inverse() {
//}

BMatrix BMatrix::transpose() {
	BMatrix ret(height, width);
	for (size_t x = 0; x < width; x++)
		for (size_t y = 0; y < height; y++)
		{
			ret.data[x][y] = data[y][x];
		}
	return ret;
}

BMatrix BMatrix::operator*(const BMatrix& other) {
	if (other.height != width) {
		std::cout << "Invalid matrix sizes for multiplication" << std::endl;
	}
	BMatrix ret(other.width, height);

	for (size_t i = 0; i < other.width; i++)
		for (size_t j = 0; j < height; j++)
			for (size_t k = 0; k < width; k++)
			{
				ret.data[i][j] += data[i][k] * other.data[k][j];
			}
	return ret;
}

//void Jacobian::operator=(const Jacobian& other) {
//	resize(other.width, other.height);
//	for (size_t x = 0; x < width; x++)
//		for (size_t y = 0; y < height; y++)
//		{
//			data[x * height + y] = other.data[x * height + y];
//		}
//}