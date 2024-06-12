#include "CW2PPMImg.h"

#include <string>
#include <QImage>
#include <GL/glu.h>

// Constructor takes a fileName and attempts to load that file as an image
CW2PPMImg::CW2PPMImg(std::string fileName) {
	// Load the file and get it's metadata using Qt
	QImage* img = new QImage(QString(fileName.c_str()));
	w = img->width();
	h = img->height();

	// Init an array to store the GL readable image data
    GLuint dataLen = w*h*3;
	data = new GLubyte[dataLen];

	// For every pixel in the image
	for (int i = 0; i < w; i++)
	{
		for (int j = 0; j < h; j++)
		{
			// Get the colour information for each pixel
			// NOTE: h-j-1 is used to read the image upside down, so that openGL will recognise it correctly
			QRgb colour = img->pixel(i,h-j-1);
			// Place it in the GL readable array
			data[(3*(j + h*i) )] = qRed(colour);	
			data[(3*(j + h*i) + 1)] = qGreen(colour);	
			data[(3*(j + h*i) + 2)] = qBlue(colour);	
		}
	}
	// Cleanup th eimg object
    delete img;
}

// Destructor to clean up allocated memory
CW2PPMImg::~CW2PPMImg() {
    delete[] data;
}