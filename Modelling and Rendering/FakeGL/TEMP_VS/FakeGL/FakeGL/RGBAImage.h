//////////////////////////////////////////////////////////////////////
//
//  University of Leeds
//  COMP 5812M Foundations of Modelling & Rendering
//  User Interface for Coursework
//
//  September, 2020
//
//  ------------------------
//  RGBAImage.h
//  ------------------------
//  
//  A minimal class for an image in single-byte RGBA format
//  Optimized for simplicity, not speed or memory
//  With read/write for ASCII RGBA files
//  
///////////////////////////////////////////////////

#pragma once

#include "glm/glm.hpp"
#include <iostream>

// the class itself
class RGBAImage
{ // class RGBAImage
public:
	//  the raw data
	glm::vec4* block;

	// dimensions of the image
	long width, height;

	// constructor
	RGBAImage();

	// copy constructor
	RGBAImage(const RGBAImage& other);

	// destructor
	~RGBAImage();

	// resizes the image, destroying any contents
	bool Resize(long Width, long Height);

	// indexing - retrieves the beginning of a line
	// array indexing will then retrieve an element
	glm::vec4* operator [](const int rowIndex);

	// similar routine for const pointers
	const glm::vec4* operator [](const int rowIndex) const;

	// routines for stream read & write
	bool ReadPPM(std::istream& inStream);
	void WritePPM(std::ostream& outStream);

}; // class RGBAImage
