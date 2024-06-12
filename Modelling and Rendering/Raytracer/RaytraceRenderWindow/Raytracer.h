#ifndef RAYTRACER_H
#define RAYTRACER_H

#include "Cartesian3.h"
#include "Homogeneous4.h"
#include "Matrix4.h"
#include "RGBAImage.h"
#include "FRGBAValue.h"
#include <vector>
#include <deque>
#include <stack>

// class constants
// bitflag constants for Clear()
const unsigned int RT_COLOR_BUFFER_BIT = 1;
const unsigned int RT_DEPTH_BUFFER_BIT = 2;
// constants for Enable()/Disable()
const unsigned int RT_LIGHTING = 1;
const unsigned int RT_TEXTURE_2D = 2;
const unsigned int RT_SHADOWS = 3;
const unsigned int RT_IMPULSE_REFLECTION = 4;
// constants for Light() - actually bit flags
const unsigned int RT_POSITION = 1;
const unsigned int RT_AMBIENT = 2;
const unsigned int RT_DIFFUSE = 4;
const unsigned int RT_AMBIENT_AND_DIFFUSE = 6;
const unsigned int RT_SPECULAR = 8;
// additional constants for Material()
const unsigned int RT_EMISSION = 16;
const unsigned int RT_SHININESS = 32;
// constants for matrix operations
const unsigned int RT_MODELVIEW = 1;
const unsigned int RT_PROJECTION = 2;
// constants for texture operations
const unsigned int RT_MODULATE = 1;
const unsigned int RT_REPLACE = 2;

class MatComponent {
    public:
    float data[4];

    MatComponent() {
        for (size_t i = 0; i < 3; i++) data[i] = 0.;
        data[3] = 1.;
    }

    MatComponent(float r, float g, float b, float a) {
        data[0] = r;
        data[1] = g;
        data[2] = b;
        data[3] = a;
    }
    MatComponent(float rgba[4]) {
        for (size_t i = 0; i < 4; i++) data[i] = rgba[i];
    }

    MatComponent operator=(float rgba[4]) {
        for (size_t i = 0; i < 4; i++) data[i] = rgba[i];
        return *this;
    }

    MatComponent operator=(const float rgba[4]) {
        for (size_t i = 0; i < 4; i++) data[i] = rgba[i];
        return *this;
    }

    MatComponent operator*(float mult) {
        float d[4];
        for (size_t i = 0; i < 4; i++) d[i] = data[i] * mult;
        return MatComponent(d);
    }
    MatComponent operator*(MatComponent other) {
        float d[4];
        for (size_t i = 0; i < 4; i++) d[i] = data[i] * other[i];
        return MatComponent(d);
    }

    MatComponent operator+(MatComponent other) {
        float d[4];
        for (size_t i = 0; i < 4; i++) d[i] = data[i] + other[i];
        return MatComponent(d);
    }

    MatComponent operator+= (MatComponent other) {
        for (size_t i = 0; i < 4; i++) data[i] += other[i];
        return *this;
    }

    float& operator[](int i) {
        return data[i];
    }
};

class Material {
    public:
    MatComponent ambient;
    MatComponent specular;
    MatComponent diffuse;
    MatComponent emission;
    float shininess = 0.;

    Material(MatComponent amb, MatComponent spec, MatComponent diff, MatComponent emis, float shin) {
        ambient = amb;
        specular = spec;
        diffuse = diff;
        emission = emis;
        shininess = shin;
    }
    Material() {}

    Material operator *(float multi) {
        MatComponent a, s, d, e;

        a = ambient * multi;
        s = specular * multi;
        d = diffuse * multi;
        e = emission * multi;
        
        return Material(a,s,d,e,shininess);
    }

    Material operator +(Material other) {
        return Material(ambient + other.ambient,
                        specular + other.specular,
                        diffuse + other.diffuse,
                        emission + other.emission,
                        shininess+other.shininess);
    }
};

// class with vertex attributes
class vertexWithAttributes
    { // class vertexWithAttributes
    public:
	// Position in ECS
    Cartesian3 position;
	// Colour
    FRGBAValue colour;
    // Normal in ECS
    Cartesian3 normal;

    // Material properties
    Material material;

    // Texture properties
    Cartesian3 texCoord;

    bool impulse;

    }; // class vertexWithAttributes

class eyeSpaceTriangle {
    public:
    vertexWithAttributes *v1,*v2,*v3;
    bool textured = false;
    bool impulse = false;
};

class ray {
    public:
    Cartesian3 origin;
    Cartesian3 dir;
};

class surfel {
    public: 
    eyeSpaceTriangle *tri=NULL;
    float alpha=0, beta=0, gamma=0;
    float distance=0;

    surfel(eyeSpaceTriangle* _tri, float _alpha, float _beta, float _gamma, float _distance) {
        tri = _tri;
        alpha = _alpha;
        beta = _beta;
        gamma = _gamma;
        distance = _distance;
    }

    surfel() {}

    Cartesian3 getPos() {
        return tri->v1->position * alpha + tri->v2->position * beta + tri->v3->position * gamma;
    }

    Cartesian3 getNorm() {
        return tri->v1->normal * alpha + tri->v2->normal * beta + tri->v3->normal * gamma;
    }
};

// the class storing the FakeGL context
class Raytracer
    { // class FakeGL
    // for the sake of simplicity & clarity, we make everything public
    public:
    //-----------------------------
    // MATRIX STATE
    //-----------------------------

    // Model View Matrix
    std::stack<Matrix4> mvMatrixStack;

    //-----------------------------
    // ATTRIBUTE STATE
    //-----------------------------

    bool shadowsEnabled = false;
    bool dBufferingEnabled = false;
    FRGBAValue drawColor;

    float windowX=0 , windowY=0,
          windowWidth=0 , windowHeight=0,
          dNear=0. , dFar=1.;

    //-----------------------------
    // OUTPUT FROM INPUT STAGE
    // INPUT TO TRANSFORM STAGE
    //-----------------------------

    // we want a queue of vertices with attributes for passing to the rasteriser
    std::deque<vertexWithAttributes> vertexQueue;

    //-----------------------------
    // TRANSFORM/LIGHTING STATE
    //-----------------------------

    Cartesian3 normal;

    bool lightingEnabled = false;
    Homogeneous4 lightPosition;

    Material lightMat;

    Material matCol;

    bool impulseEnabled = false;

    bool impulseVert = false;

    //-----------------------------
    // WORLD PRIMITIVES
    //-----------------------------

    std::vector<vertexWithAttributes> vertexVect;
    std::vector<eyeSpaceTriangle> triangleVect;

    //-----------------------------
    // TEXTURE STATE
    //-----------------------------

    bool textureEnabled = false;
    unsigned texMode = 0;
    Cartesian3 texCoord;
    const RGBAImage *texture;

    //-----------------------------
    // FRAMEBUFFER STATE
    // OUTPUT FROM FRAGMENT STAGE
    //-----------------------------
    
    // Clear will be done when the world state changes, however not every frame
    FRGBAValue fbClearColor;

	// the frame buffer itself
    RGBAImage frameBuffer;
     
    // Depth buffer is currently unusued
    RGBAImage depthBuffer;
    
    //-------------------------------------------------//
    //                                                 //
    // CONSTRUCTOR / DESTRUCTOR                        //
    //                                                 //
    //-------------------------------------------------//
    
    // constructor
    Raytracer();
    
    // destructor
    ~Raytracer();
    
    //-------------------------------------------------//
    //                                                 //
    // GEOMETRIC PRIMITIVE ROUTINES                    //
    //                                                 //
    //-------------------------------------------------//
    
    // starts a sequence of geometric primitives
    void Begin();
    
    // ends a sequence of geometric primitives
    void End();
    
    //-------------------------------------------------//
    //                                                 //
    // MATRIX MANIPULATION ROUTINES                    //
    //                                                 //
    //-------------------------------------------------//


    // pushes a matrix on the stack
    void PushMatrix();

    // pops a matrix off the stack
    void PopMatrix();

    // load the identity matrix
    void LoadIdentity();
    
    // multiply by a known matrix in column-major format
    void MultMatrixf(const float *columnMajorCoordinates);

    // Change these to occomated camera position and such for raytracer

    // sets a perspective projection matrix
    void Frustum(float left, float right, float bottom, float top, float zNear, float zFar);

    // sets an orthographic projection matrix
    void Ortho(float left, float right, float bottom, float top, float zNear, float zFar);

    // rotate the matrix
    void Rotatef(float angle, float axisX, float axisY, float axisZ);

    // scale the matrix
    void Scalef(float xScale, float yScale, float zScale);
    
    // translate the matrix
    void Translatef(float xTranslate, float yTranslate, float zTranslate);
    
    // sets the viewport
    // WILL NEED CHANGING FOR RT
    void Viewport(int x, int y, int width, int height);

    //-------------------------------------------------//
    //                                                 //
    // VERTEX ATTRIBUTE ROUTINES                       //
    //                                                 //
    //-------------------------------------------------//

    // sets colour with floating point
    void Color3f(float red, float green, float blue);
    
    // sets material properties
    void Materialf(unsigned int parameterName, const float parameterValue);
    void Materialfv(unsigned int parameterName, const float *parameterValues);

    // sets the normal vector
    void Normal3f(float x, float y, float z);

    // sets the texture coordinates
    void TexCoord2f(float u, float v);

    // sets the vertex & launches it down the pipeline
    void Vertex3f(float x, float y, float z);

    void SetImpulse(bool state);

    //-------------------------------------------------//
    //                                                 //
    // STATE VARIABLE ROUTINES                         //
    //                                                 //
    //-------------------------------------------------//

    // disables a specific flag in the library
    void Disable(unsigned int property);
    
    // enables a specific flag in the library
    void Enable(unsigned int property);
    
    //-------------------------------------------------//
    //                                                 //
    // LIGHTING STATE ROUTINES                         //
    //                                                 //
    //-------------------------------------------------//

    // sets properties for the one and only light
    void Light(int parameterName, const float *parameterValues);

    //-------------------------------------------------//
    //                                                 //
    // TEXTURE PROCESSING ROUTINES                     //
    //                                                 //
    // Note that we only allow one texture             //
    // so glGenTexture & glBindTexture aren't needed   //
    //                                                 //
    //-------------------------------------------------//

    // sets whether textures replace or modulate
    void TexEnvMode(unsigned int textureMode);

    // sets the texture image that corresponds to a given ID
    void TexImage2D(const RGBAImage &textureImage);

    //-------------------------------------------------//
    //                                                 //
    // FRAME BUFFER ROUTINES                           //
    //                                                 //
    //-------------------------------------------------//

    // clears the frame buffer
    void Clear(unsigned int mask);
    
    // sets the clear colour for the frame buffer
    void ClearColor(float red, float green, float blue, float alpha);

    //-------------------------------------------------//
    //                                                 //
    // MAJOR PROCESSING ROUTINES                       //
    //                                                 //
    //-------------------------------------------------//

    // Will include a ray tracer
    // Render ruitine

    void drawScreenByPix();
    surfel RayCast(ray r);

    void drawScreenByTri();
    surfel RayCast(ray r, eyeSpaceTriangle *triangle);

    bool RayTriIntersectTest(ray r, eyeSpaceTriangle* tri, surfel &intersec);

    void getBarycentric(Cartesian3 p, Cartesian3 a, Cartesian3 b, Cartesian3 c, float &alpha, float &beta, float &gamma);
    
    void RenderIntersec(surfel intersec, size_t x, size_t y);

    }; // class FakeGL

float clamp(float,float,float);
Cartesian3 reflectVector(Cartesian3 v, Cartesian3 n);

// standard routine for dumping the entire FakeGL context (except for texture / image)
// std::ostream &operator << (std::ostream &outStream, Raytracer &fakeGL); 

// include guard        
#endif