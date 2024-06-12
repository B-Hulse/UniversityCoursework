#ifndef __CW2_MATSTRUCT_H__
#define __CW2_MATSTRUCT_H__ 1

#include <GL/glu.h>

// A structure to hold the properties of materials
typedef struct matStruct
{
  GLfloat ambient[4];
  GLfloat diffuse[4];
  GLfloat specular[4];
  GLfloat shininess;
} matStruct;

#endif