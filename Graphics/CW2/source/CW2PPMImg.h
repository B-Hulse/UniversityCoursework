#ifndef __CW2_PPMIMG_H__
#define __CW2_PPMIMG_H__

#include <string>
#include <GL/glu.h>

// A simple class to read an image from file, and store it in a way that can be read by openGL
class CW2PPMImg {
    public:
    CW2PPMImg(std::string fileName);
    ~CW2PPMImg();
    // Basic getters for private attributes
    int W() const  {return h;}
    int H() const {return w;}
    GLubyte* Data() const {return data;}

    private:
    int w;
    int h;
    GLubyte* data;
};

#endif