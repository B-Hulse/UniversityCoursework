//////////////////////////////////////////////////////////////////////
//
//  University of Leeds
//  COMP 5812M Foundations of Modelling & Rendering
//  User Interface for Coursework
//
//  September, 2020
//
//  ------------------------
//  FakeGL.cpp
//  ------------------------
//  
//  A unit for implementing OpenGL workalike calls
//  
///////////////////////////////////////////////////
 
#include "FakeGL.h"
#include <math.h>
#include <algorithm>

//-------------------------------------------------//
//                                                 //
// CONSTRUCTOR / DESTRUCTOR                        //
//                                                 //
//-------------------------------------------------//

// constructor
FakeGL::FakeGL()
    { // constructor
        // Initialise the Matrix stacks with empty matrices
        mvMatrixStack.push(Matrix4());
        pMatrixStack.push(Matrix4());
    } // constructor

// destructor
FakeGL::~FakeGL()
    { // destructor
    } // destructor

//-------------------------------------------------//
//                                                 //
// GEOMETRIC PRIMITIVE ROUTINES                    //
//                                                 //
//-------------------------------------------------//

// starts a sequence of geometric primitives
void FakeGL::Begin(unsigned int PrimitiveType)
    { // Begin()
        primType = PrimitiveType;
        
        // Clear the queues incase too many vertices were listed in the previous Begin & End call pair
        vertexQueue.clear();
        rasterQueue.clear();
        fragmentQueue.clear();
    } // Begin()

// ends a sequence of geometric primitives
void FakeGL::End()
    { // End()
        // Tranform every vertex, TransformVertex() reduces the vertexQueue size by 1 each call
        for (; vertexQueue.size() > 0;TransformVertex());

        // Rasterise primitives until there are not enough vertices in the queue
        while (RasterisePrimitive());

        // Process each fragment one by one
        for (; fragmentQueue.size() > 0; ProcessFragment());
    } // End()

// sets the size of a point for drawing
void FakeGL::PointSize(float size)
    { // PointSize()
        pointSize = (unsigned int) size;
    } // PointSize()

// sets the width of a line for drawing purposes
void FakeGL::LineWidth(float width)
    { // LineWidth()
        lineWidth = width;
    } // LineWidth()

//-------------------------------------------------//
//                                                 //
// MATRIX MANIPULATION ROUTINES                    //
//                                                 //
//-------------------------------------------------//

// set the matrix mode (i.e. which one we change)   
void FakeGL::MatrixMode(unsigned int whichMatrix)
    { // MatrixMode()
        // currMatrixStack is a pointer
        if (whichMatrix == FAKEGL_MODELVIEW) {
            currMatrixStack = &mvMatrixStack;
        }
        if (whichMatrix == FAKEGL_PROJECTION) {
            currMatrixStack = &pMatrixStack;
        }
    } // MatrixMode()

// pushes a matrix on the stack
void FakeGL::PushMatrix()
    { // PushMatrix()
        // Copies the top of the stack and pushes it on
        currMatrixStack->push(currMatrixStack->top());
    } // PushMatrix()

// pops a matrix off the stack
void FakeGL::PopMatrix()
    { // PopMatrix()
        currMatrixStack->pop();
        // If the pop results in an empty stack, fill with an empty matrix
        if (currMatrixStack->size() == 0) {
            currMatrixStack->push(Matrix4());
        }
    } // PopMatrix()

// load the identity matrix
void FakeGL::LoadIdentity()
    { // LoadIdentity()
        currMatrixStack->top().SetIdentity();
    } // LoadIdentity()

// multiply by a known matrix in column-major format
void FakeGL::MultMatrixf(const float *columnMajorCoordinates)
    { // MultMatrixf()

        // Convert the matrix passed in to a Matrix4 instance
        Matrix4 inMat;
        for (size_t i = 0; i < 16; i++)
        {
            inMat[i%4][(size_t)floor(i/4)]=columnMajorCoordinates[i];
        }

        // Multiply the stack by said matrix
        currMatrixStack->top() = currMatrixStack->top() * inMat;
        
    } // MultMatrixf()

// sets up a perspective projection matrix
void FakeGL::Frustum(float left, float right, float bottom, float top, float zNear, float zFar)
    { // Frustum()
        // Generate the frustrum projection matrix according to the OpenGL documentation
        Matrix4 frusMat;
        frusMat[0][0] = (2.*zNear)/(right-left);
        frusMat[1][1] = (2.*zNear)/(top-bottom);

        frusMat[0][2] = (right + left)/(right - left);
        frusMat[1][2] = (top + bottom)/(top - bottom);
        frusMat[2][2] = -1 * (zFar + zNear)/(zFar - zNear);
        frusMat[3][2] = -1;

        frusMat[2][3] = -1 * (2 * zFar * zNear)/(zFar - zNear);

        dNear = zNear; dFar = zFar;

        currMatrixStack->top() = currMatrixStack->top() * frusMat;
    } // Frustum()

// sets an orthographic projection matrix
void FakeGL::Ortho(float left, float right, float bottom, float top, float zNear, float zFar)
    { // Ortho()
        // Same as frustrum but for orthographic projection
        Matrix4 orthMat;
        orthMat[0][0] = 2./(right-left);
        orthMat[1][1] = 2./(top-bottom);
        orthMat[2][2] = (-2.)/(zFar-zNear);
        orthMat[3][3] = 1;

        orthMat[0][3] = -1 * ((right+left)/(right-left));
        orthMat[1][3] = -1 * ((top+bottom)/(top-bottom));
        orthMat[2][3] = -1 * ((zFar+zNear)/(zFar-zNear));

        dNear = zNear; dFar = zFar;

        currMatrixStack->top() = currMatrixStack->top() * orthMat;
    } // Ortho()

// rotate the matrix
void FakeGL::Rotatef(float angle, float axisX, float axisY, float axisZ)
    { // Rotatef()
        // Create the desired rotation as a matrix
        Matrix4 rotMat;
        rotMat.SetRotation(Cartesian3(axisX,axisY,axisZ),angle);

        currMatrixStack->top() = currMatrixStack->top() * rotMat;
    } // Rotatef()

// scale the matrix
void FakeGL::Scalef(float xScale, float yScale, float zScale)
    { // Scalef()
        Matrix4 scaleMat;
        scaleMat.SetScale(xScale,yScale,zScale);

        currMatrixStack->top() = currMatrixStack->top() * scaleMat;
    } // Scalef()

// translate the matrix
void FakeGL::Translatef(float xTranslate, float yTranslate, float zTranslate)
    { // Translatef()
        Matrix4 transMat;
        transMat.SetTranslation(Cartesian3(xTranslate,yTranslate,zTranslate));

        currMatrixStack->top() = currMatrixStack->top() * transMat;
    } // Translatef()

// sets the viewport
void FakeGL::Viewport(int x, int y, int width, int height)
    { // Viewport()
        windowX = x;
        windowY = y;
        windowWidth = width;
        windowHeight = height;
    } // Viewport()

//-------------------------------------------------//
//                                                 //
// VERTEX ATTRIBUTE ROUTINES                       //
//                                                 //
//-------------------------------------------------//

// sets colour with floating point
void FakeGL::Color3f(float red, float green, float blue)
    { // Color3f()
        // Scale up the floats from 0-1 -> 0->255
        drawColor.red = red*255.f;
        drawColor.green = green*255.f;
        drawColor.blue = blue*255.f;
        drawColor.alpha = 255.f;
    } // Color3f()

// sets material properties
void FakeGL::Materialf(unsigned int parameterName, const float parameterValue)
    { // Materialf()
        if (parameterName & FAKEGL_SHININESS) {
            matShininess = parameterValue;
        }
    } // Materialf()

void FakeGL::Materialfv(unsigned int parameterName, const float *parameterValues)
    { // Materialfv()

        if (parameterName & FAKEGL_AMBIENT_AND_DIFFUSE) {
            for (size_t i = 0; i < 4; i++)
            {
                matAmbient[i] = parameterValues[i];
                matDiffuse[i] = parameterValues[i];
            }
        }
        if (parameterName & FAKEGL_AMBIENT) {
            for (size_t i = 0; i < 4; i++)
            {
                matAmbient[i] = parameterValues[i];
            }
        }
        if (parameterName & FAKEGL_DIFFUSE) {
            for (size_t i = 0; i < 4; i++)
            {
                matDiffuse[i] = parameterValues[i];
            }
        }
        if (parameterName & FAKEGL_SPECULAR) {
            for (size_t i = 0; i < 4; i++)
            {
                matSpecular[i] = parameterValues[i];
            }
        }
        if (parameterName & FAKEGL_EMISSION) {
            for (size_t i = 0; i < 4; i++)
            {
                matEmission[i] = parameterValues[i];
            }
        }
    } // Materialfv()

// sets the normal vector
void FakeGL::Normal3f(float x, float y, float z)
    { // Normal3f()
        normal = Cartesian3(x,y,z);
    } // Normal3f()

// sets the texture coordinates
void FakeGL::TexCoord2f(float u, float v)
    { // TexCoord2f()
    texCoord = Cartesian3(u,v,0.f);
    } // TexCoord2f()

// sets the vertex & launches it down the pipeline
void FakeGL::Vertex3f(float x, float y, float z)
    { // Vertex3f()
        // Generate a new vertex with the properties filled by the current state of the FakeGL instance
        vertexWithAttributes newVert;
        newVert.position = Homogeneous4(x,y,z);
        newVert.colour.red = drawColor.red;
        newVert.colour.green = drawColor.green;
        newVert.colour.blue = drawColor.blue;
        newVert.colour.alpha = drawColor.alpha;
        newVert.normal = normal;

        for (size_t i = 0; i < 4; i++) {
            newVert.spec[i] = matSpecular[i];
            newVert.amb[i] = matAmbient[i];
            newVert.diff[i] = matDiffuse[i];
            newVert.emiss[i] = matEmission[i];
        }
        newVert.shin = matShininess;

        newVert.texCoord = texCoord;

        // Add it to the vertex queue
        vertexQueue.push_back(newVert);
    } // Vertex3f()

//-------------------------------------------------//
//                                                 //
// STATE VARIABLE ROUTINES                         //
//                                                 //
//-------------------------------------------------//

// disables a specific flag in the library
void FakeGL::Disable(unsigned int property)
    { // Disable()
        if (property == FAKEGL_LIGHTING) {
            lightingEnabled = false;
        }
        if (property == FAKEGL_DEPTH_TEST) {
            dBufferingEnabled = false;
        }
        if (property == FAKEGL_TEXTURE_2D) {
            textureEnabled = false;
        }
        if (property == FAKEGL_PHONG_SHADING) {
            phongEnabled = false;
        }
    } // Disable()

// enables a specific flag in the library
void FakeGL::Enable(unsigned int property)
    { // Enable()
        if (property == FAKEGL_LIGHTING) {
            lightingEnabled = true;
        }
        if (property == FAKEGL_DEPTH_TEST) {
            dBufferingEnabled = true;
        }
        if (property == FAKEGL_TEXTURE_2D) {
            textureEnabled = true;
        }
        if (property == FAKEGL_PHONG_SHADING) {
            phongEnabled = true;
        }
    } // Enable()

//-------------------------------------------------//
//                                                 //
// LIGHTING STATE ROUTINES                         //
//                                                 //
//-------------------------------------------------//

// sets properties for the one and only light
void FakeGL::Light(int parameterName, const float *parameterValues)
    { // Light()
        if (parameterName & FAKEGL_POSITION) {
            lightPosition = Homogeneous4(parameterValues[0],
                                         parameterValues[1],
                                         parameterValues[2],
                                         parameterValues[3]);

            // Transform the light position by the Model View matrix so that the position is stored in Eye space (VCS)
            lightPosition = mvMatrixStack.top() * lightPosition;
        }
        if (parameterName & FAKEGL_AMBIENT) {
            for (size_t i = 0; i < 4; i++) 
            {
                // This offset allows the ambient light to look closer to normal OpenGL
                // IT will not change how the lighting is calculated, only boost the ambient light slightly
                lightAmbient[i] = parameterValues[i] + 0.2f;
            }
        }
        if (parameterName & FAKEGL_DIFFUSE) {
            for (size_t i = 0; i < 4; i++)
            {
                lightDiffuse[i] = parameterValues[i];
            }
        }
        if (parameterName & FAKEGL_SPECULAR) {
            for (size_t i = 0; i < 4; i++)
            {
                lightSpecular[i] = parameterValues[i];
            }
        }
    } // Light()

//-------------------------------------------------//
//                                                 //
// TEXTURE PROCESSING ROUTINES                     //
//                                                 //
// Note that we only allow one texture             //
// so glGenTexture & glBindTexture aren't needed   //
//                                                 //
//-------------------------------------------------//

// sets whether textures replace or modulate
void FakeGL::TexEnvMode(unsigned int textureMode)
    { // TexEnvMode()
    texMode = textureMode;
    } // TexEnvMode()

// sets the texture image that corresponds to a given ID
void FakeGL::TexImage2D(const RGBAImage &textureImage)
    { // TexImage2D()
    // Texture is a const here as it only allows one texture to be used
    texture = &textureImage;
    } // TexImage2D()

//-------------------------------------------------//
//                                                 //
// FRAME BUFFER ROUTINES                           //
//                                                 //
//-------------------------------------------------//

// clears the frame buffer
void FakeGL::Clear(unsigned int mask)
    { // Clear()        

        // If clear color buffer is set
        if (mask & FAKEGL_COLOR_BUFFER_BIT) {
            for (size_t i = 0; i < frameBuffer.height*frameBuffer.width; i++)
            {
                // Update every pixel to be the clear color
                frameBuffer.block[i].red = fbClearColor.red;
                frameBuffer.block[i].green = fbClearColor.green;
                frameBuffer.block[i].blue = fbClearColor.blue;
                frameBuffer.block[i].alpha = fbClearColor.alpha;
            }
        }
        // If clear depth buffer is set
        if (mask & FAKEGL_DEPTH_BUFFER_BIT) {
            // Reset depth buffer to max value
            for (size_t i = 0; i < depthBuffer.width*depthBuffer.height; i++)
                depthBuffer.block[i].alpha = 255;
        }
    } // Clear()

// sets the clear colour for the frame buffer
void FakeGL::ClearColor(float red, float green, float blue, float alpha)
    { // ClearColor()
        // Scale up the floats from 0-1 -> 0->255
        fbClearColor.red = red*255.f;
        fbClearColor.green = green*255.f;
        fbClearColor.blue = blue*255.f;
        fbClearColor.alpha = alpha*255.f;
    } // ClearColor()

//-------------------------------------------------//
//                                                 //
// MAJOR PROCESSING ROUTINES                       //
//                                                 //
//-------------------------------------------------//

// transform one vertex & shift to the raster queue
void FakeGL::TransformVertex()
    { // TransformVertex()
        // Get vertex from the queue
        vertexWithAttributes vert = vertexQueue.front();
        vertexQueue.pop_front();

        // Convert to VCS
        Homogeneous4 vertVCS = mvMatrixStack.top() * vert.position;

        // Compute Lighting
        if (lightingEnabled and !phongEnabled) {
            float totalLight[4] = {0.,0.,0.,0.};

            // Emissive light
            totalLight[0] += vert.emiss[0];
            totalLight[1] += vert.emiss[1];
            totalLight[2] += vert.emiss[2];
            // totalLight[3] += vert.emiss[3];

            // Ambient Light
            totalLight[0] += (vert.amb[0]*lightAmbient[0]);
            totalLight[1] += (vert.amb[1]*lightAmbient[1]);
            totalLight[2] += (vert.amb[2]*lightAmbient[2]);
            // totalLight[3] += (vert.amb[3]*lightAmbient[3]);

            // Diffuse Light
            Cartesian3 norm = mvMatrixStack.top() * vert.normal;
            Cartesian3 lightDir = lightPosition.Vector();
            float diffuse = std::max(norm.unit().dot(lightDir.unit()),0.f);

            totalLight[0] += (vert.diff[0] * lightDiffuse[0] * diffuse);
            totalLight[1] += (vert.diff[1] * lightDiffuse[1] * diffuse);
            totalLight[2] += (vert.diff[2] * lightDiffuse[2] * diffuse);
            totalLight[3] += (vert.diff[3]);

            // Specular Light
            Cartesian3 eyeDir = vertVCS.Vector();
            Cartesian3 bisecDir = (lightDir+eyeDir)/2.f;

            float specular= 0;

            // If the light is going to hit the surface directly
            if (norm.dot(lightDir) > 0) {
                float ndotvb = norm.dot(bisecDir) / (norm.length() * bisecDir.length());

                specular = ndotvb > 0 ? pow(ndotvb,vert.shin*4.) : 0;
            }
            
            totalLight[0] += (vert.spec[0] * lightSpecular[0] * specular);
            totalLight[1] += (vert.spec[1] * lightSpecular[1] * specular);
            totalLight[2] += (vert.spec[2] * lightSpecular[2] * specular);
            // totalLight[3] += (vert.spec[3] * lightSpecular[3] * specular);

            // Set vertex color to the lighting color, limiting to range [0,255]
            RGBAValue newVCol;
            newVCol.red = clamp(totalLight[0] * 255., 0.f, 255.f);
            newVCol.green = clamp(totalLight[1] * 255., 0.f, 255.f);
            newVCol.blue = clamp(totalLight[2] * 255., 0.f, 255.f);
            newVCol.alpha = clamp(totalLight[3] * 255., 0.f, 255.f);
            vert.colour = newVCol;
        }

        // Convert to CCS
        Homogeneous4 vertCCS = pMatrixStack.top()  * vertVCS;

        // Convert to NDCS
        Cartesian3 vertNDCS(vertCCS.x/vertCCS.w,vertCCS.y/vertCCS.w,vertCCS.z/vertCCS.w);

        // Convert to DCS
        Cartesian3 vertDCS;
        vertDCS.x = ((vertNDCS.x+1) * (windowWidth/2.)) + windowX;
        vertDCS.y = ((vertNDCS.y+1) * (windowHeight/2.)) + windowY;
        vertDCS.z = ((((dFar-dNear)/2.)*vertNDCS.z)+((dFar+dNear)/2.));

        // Add to raster Queue
        screenVertexWithAttributes sVert;
        sVert.colour = vert.colour;
        sVert.position = Cartesian3(vertDCS.x,vertDCS.y,vertDCS.z);
        sVert.texCoord = vert.texCoord;

        // Copy normal and material properties
        sVert.normal = vert.normal;
        for (size_t i = 0; i < 4; i++)
        {
            sVert.amb[i] = vert.amb[i];
            sVert.diff[i] = vert.diff[i];
            sVert.spec[i] = vert.spec[i];
            sVert.emiss[i] = vert.emiss[i];
        }
        sVert.shin = vert.shin;
        
        sVert.ePos = vertVCS;

        rasterQueue.push_back(sVert);
    } // TransformVertex()

// rasterise a single primitive if there are enough vertices on the queue
bool FakeGL::RasterisePrimitive()
    { // RasterisePrimitive()
    
        // One code path for each primitive type

        if (primType == FAKEGL_POINTS) {
            // If there are enough vertices queued for a point (1 vertex)
            if (rasterQueue.size() < 1) return false;
            // Rasterise the point and remove the processed vertex from the queue
            RasterisePoint(rasterQueue.front());
            rasterQueue.pop_front();
        }
        if (primType == FAKEGL_LINES) {
            if (rasterQueue.size() < 2) return false;
            screenVertexWithAttributes v0,v1;
            v0 = rasterQueue.front(); rasterQueue.pop_front();
            v1 = rasterQueue.front(); rasterQueue.pop_front();
            RasteriseLineSegment(v0,v1);
        }
        if (primType == FAKEGL_TRIANGLES) {
            if (rasterQueue.size() < 3) return false;
            screenVertexWithAttributes v0,v1,v2;
            v0 = rasterQueue.front(); rasterQueue.pop_front();
            v1 = rasterQueue.front(); rasterQueue.pop_front();
            v2 = rasterQueue.front(); rasterQueue.pop_front();
            RasteriseTriangle(v0,v1,v2);

        }
        return true;
    } // RasterisePrimitive()

// rasterises a single point
void FakeGL::RasterisePoint(screenVertexWithAttributes &vertex0)
    { // RasterisePoint()
        // Generate a point fragment that is at the position of the vertex
        fragmentWithAttributes newFrag;
        newFrag.colour = vertex0.colour;
        newFrag.depth = vertex0.position.z;
        newFrag.normal = vertex0.normal;

        // Find point 'radius'
        unsigned int halfPSize = (unsigned int) floor(pointSize/2.);
        // Add a fragment for each pixel in the point to the fragment queue
        for (size_t xi = 0; xi < pointSize; xi++)
            for (size_t yi = 0; yi < pointSize; yi++)
            {
                newFrag.row = vertex0.position.y - halfPSize+yi;
                newFrag.col = vertex0.position.x - halfPSize+xi;
                fragmentQueue.push_back(newFrag);
            }
        

    } // RasterisePoint()

// rasterises a single line segment
void FakeGL::RasteriseLineSegment(screenVertexWithAttributes &vertex0, screenVertexWithAttributes &vertex1)
    { // RasteriseLineSegment()
        Cartesian3 vector01 = vertex1.position - vertex0.position;
        Cartesian3 normal01(-vector01.y, vector01.x, 0.0);
        // normalise
        normal01 = normal01.unit();
        // The four corners of the rectangle that will take place of the line
        screenVertexWithAttributes c0,c1,c2,c3;

        float halfLineWidth = ((float)lineWidth) / 2.f;
        // Modifier for the width if it is an even number
        float widthMod = (lineWidth%2==0 ? -1 : 0);

        // Set the positions of the four corners
        c0.position = vertex0.position + (normal01 * (halfLineWidth+widthMod));
        c1.position = vertex0.position - (normal01 * (halfLineWidth));
        c0.colour = vertex0.colour; c1.colour = vertex0.colour;
        c0.normal = vertex0.normal; c1.normal = vertex0.normal;

        c2.position = vertex1.position - (normal01 * halfLineWidth);
        c3.position = vertex1.position + (normal01 * (halfLineWidth+widthMod));
        c2.colour = vertex1.colour; c3.colour = vertex1.colour;
        c2.normal = vertex1.normal; c3.normal = vertex1.normal;

        // Copy color properties
        for (size_t i = 0; i < 4; i++)
        {
            c0.amb[i] = vertex0.amb[i];
            c0.diff[i] = vertex0.diff[i];
            c0.spec[i] = vertex0.spec[i];
            c0.emiss[i] = vertex0.emiss[i];
            c1.amb[i] = vertex0.amb[i];
            c1.diff[i] = vertex0.diff[i];
            c1.spec[i] = vertex0.spec[i];
            c1.emiss[i] = vertex0.emiss[i];
            c2.amb[i] = vertex1.amb[i];
            c2.diff[i] = vertex1.diff[i];
            c2.spec[i] = vertex1.spec[i];
            c2.emiss[i] = vertex1.emiss[i];
            c3.amb[i] = vertex1.amb[i];
            c3.diff[i] = vertex1.diff[i];
            c3.spec[i] = vertex1.spec[i];
            c3.emiss[i] = vertex1.emiss[i];
        }
        c0.shin = vertex0.shin;
        c1.shin = vertex0.shin;
        c2.shin = vertex1.shin;
        c3.shin = vertex1.shin;
        

        // rasterise the triangles using the four corners
        RasteriseTriangle(c0,c1,c2);
        RasteriseTriangle(c0,c2,c3);

    } // RasteriseLineSegment()

// rasterises a single triangle
void FakeGL::RasteriseTriangle(screenVertexWithAttributes &vertex0, screenVertexWithAttributes &vertex1, screenVertexWithAttributes &vertex2)
    { // RasteriseTriangle()
    // compute a bounding box that starts inverted to frame size
    // clipping will happen in the raster loop proper
    float minX = frameBuffer.width, maxX = 0.0;
    float minY = frameBuffer.height, maxY = 0.0;
    
    // test against all vertices
    if (vertex0.position.x < minX) minX = vertex0.position.x;
    if (vertex0.position.x > maxX) maxX = vertex0.position.x;
    if (vertex0.position.y < minY) minY = vertex0.position.y;
    if (vertex0.position.y > maxY) maxY = vertex0.position.y;
    
    if (vertex1.position.x < minX) minX = vertex1.position.x;
    if (vertex1.position.x > maxX) maxX = vertex1.position.x;
    if (vertex1.position.y < minY) minY = vertex1.position.y;
    if (vertex1.position.y > maxY) maxY = vertex1.position.y;
    
    if (vertex2.position.x < minX) minX = vertex2.position.x;
    if (vertex2.position.x > maxX) maxX = vertex2.position.x;
    if (vertex2.position.y < minY) minY = vertex2.position.y;
    if (vertex2.position.y > maxY) maxY = vertex2.position.y;

    // now for each side of the triangle, compute the line vectors
    Cartesian3 vector01 = vertex1.position - vertex0.position;
    Cartesian3 vector12 = vertex2.position - vertex1.position;
    Cartesian3 vector20 = vertex0.position - vertex2.position;

    // now compute the line normal vectors
    Cartesian3 normal01(-vector01.y, vector01.x, 0.0);  
    Cartesian3 normal12(-vector12.y, vector12.x, 0.0);  
    Cartesian3 normal20(-vector20.y, vector20.x, 0.0);  

    // we don't need to normalise them, because the square roots will cancel out in the barycentric coordinates
    float lineConstant01 = normal01.dot(vertex0.position);
    float lineConstant12 = normal12.dot(vertex1.position);
    float lineConstant20 = normal20.dot(vertex2.position);

    // and compute the distance of each vertex from the opposing side
    float distance0 = normal12.dot(vertex0.position) - lineConstant12;
    float distance1 = normal20.dot(vertex1.position) - lineConstant20;
    float distance2 = normal01.dot(vertex2.position) - lineConstant01;

    // if any of these are zero, we will have a divide by zero error
    // but notice that if they are zero, the vertices are collinear in projection and the triangle is edge on
    // we can render that as a line, but the better solution is to render nothing.  In a surface, the adjacent
    // triangles will eventually take care of it
    if ((distance0 == 0) || (distance1 == 0) || (distance2 == 0))
        return; 

    // create a fragment for reuse
    fragmentWithAttributes rasterFragment;

    // loop through the pixels in the bounding box
    for (rasterFragment.row = minY; rasterFragment.row <= maxY; rasterFragment.row++)
        { // per row
        // this is here so that clipping works correctly
        if (rasterFragment.row < 0) continue;
        if (rasterFragment.row >= frameBuffer.height) continue;
        for (rasterFragment.col = minX; rasterFragment.col <= maxX; rasterFragment.col++)
            { // per pixel
            // this is also for correct clipping
            if (rasterFragment.col < 0) continue;
            if (rasterFragment.col >= frameBuffer.width) continue;
            
            // the pixel in cartesian format
            Cartesian3 pixel(rasterFragment.col, rasterFragment.row, 0.);
            
            // right - we have a pixel inside the frame buffer AND the bounding box
            // note we *COULD* compute gamma = 1.0 - alpha - beta instead
            float alpha = (normal12.dot(pixel) - lineConstant12) / distance0;           
            float beta = (normal20.dot(pixel) - lineConstant20) / distance1;            
            float gamma = (normal01.dot(pixel) - lineConstant01) / distance2;           

            // now perform the half-plane test
            if ((alpha < 0.0) || (beta < 0.0) || (gamma < 0.0))
                continue;

            // compute colour
            rasterFragment.colour = alpha * vertex0.colour + beta * vertex1.colour + gamma * vertex2.colour; 

            if (lightingEnabled and phongEnabled) {
                // Interpolate normal and material properties
                Cartesian3 fragNormal = alpha * vertex0.normal + beta * vertex1.normal + gamma * vertex2.normal;
                float fragAmb[4];
                float fragDiff[4];
                float fragSpec[4];
                float fragEmiss[4]; 
                for (size_t i = 0; i < 4; i++)
                {
                    fragAmb[i] = alpha * vertex0.amb[i] + beta * vertex1.amb[i] + gamma * vertex2.amb[i];
                    fragDiff[i] = alpha * vertex0.diff[i] + beta * vertex1.diff[i] + gamma * vertex2.diff[i];
                    fragSpec[i] = alpha * vertex0.spec[i] + beta * vertex1.spec[i] + gamma * vertex2.spec[i];
                    fragEmiss[i] = alpha * vertex0.emiss[i] + beta * vertex1.emiss[i] + gamma * vertex2.emiss[i];
                }
                float fragShin = alpha * vertex0.shin + beta * vertex1.shin + gamma * vertex2.shin;
                Cartesian3 fragEPos = alpha * vertex0.ePos.Vector() + beta * vertex1.ePos.Vector() + gamma * vertex2.ePos.Vector();

                float totalLight[4] = {0.,0.,0.,0.};

                // Emissive light
                totalLight[0] += fragEmiss[0];
                totalLight[1] += fragEmiss[1];
                totalLight[2] += fragEmiss[2];
                // totalLight[3] += vert.emiss[3];

                // Ambient Light
                totalLight[0] += (fragAmb[0]*lightAmbient[0]);
                totalLight[1] += (fragAmb[1]*lightAmbient[1]);
                totalLight[2] += (fragAmb[2]*lightAmbient[2]);
                // totalLight[3] += (vert.amb[3]*lightAmbient[3]);

                // Diffuse Light
                Cartesian3 norm = mvMatrixStack.top() * fragNormal;
                Cartesian3 lightDir = lightPosition.Vector();
                float diffuse = std::max(norm.unit().dot(lightDir.unit()),0.f);

                totalLight[0] += (fragDiff[0] * lightDiffuse[0] * diffuse);
                totalLight[1] += (fragDiff[1] * lightDiffuse[1] * diffuse);
                totalLight[2] += (fragDiff[2] * lightDiffuse[2] * diffuse);
                totalLight[3] += (fragDiff[3]);

                // Specular Light
                Cartesian3 eyeDir = fragEPos;
                Cartesian3 bisecDir = (lightDir+eyeDir)/2.f;

                float specular= 0;

                // If the light is going to hit the surface directly
                if (norm.dot(lightDir) > 0) {
                    float ndotvb = norm.dot(bisecDir) / (norm.length() * bisecDir.length());

                    specular = ndotvb > 0 ? pow(ndotvb,fragShin*4.) : 0;
                }
                
                totalLight[0] += (fragSpec[0] * lightSpecular[0] * specular);
                totalLight[1] += (fragSpec[1] * lightSpecular[1] * specular);
                totalLight[2] += (fragSpec[2] * lightSpecular[2] * specular);
                // totalLight[3] += (vert.spec[3] * lightSpecular[3] * specular);

                // Set vertex color to the lighting color, limiting to range [0,255]
                RGBAValue newVCol;
                newVCol.red = clamp(totalLight[0] * 255., 0.f, 255.f);
                newVCol.green = clamp(totalLight[1] * 255., 0.f, 255.f);
                newVCol.blue = clamp(totalLight[2] * 255., 0.f, 255.f);
                newVCol.alpha = clamp(totalLight[3] * 255., 0.f, 255.f);
                rasterFragment.colour = newVCol;
            }

            if (textureEnabled) {
                // Calculate the position in the texture of the fragment using barycentric [0,1]        
                Cartesian3 fragTexCoord = alpha * vertex0.texCoord + beta * vertex1.texCoord + gamma * vertex2.texCoord; 
                // Convert to texel coordinates
                size_t texIndexIx = (size_t)(fragTexCoord.x * texture->width); 
                size_t texIndexIy = (size_t)(fragTexCoord.y * texture->height); 

                // This is to prevent a rare segmentation fault that  I think is caused by an attempt to access a texel outside fo the textures range
                if (texIndexIx < texture->width and texIndexIx >= 0 and texIndexIy < texture->height and texIndexIy >= 0) {
                    if (texMode == FAKEGL_REPLACE) {
                        // Replace all color/lighting with the texel color
                        rasterFragment.colour = texture->block[texIndexIy*texture->width + texIndexIx];
                    } else if (texMode == FAKEGL_MODULATE) {
                        // Map the texel color to [0,1]
                        RGBAValue texCol = texture->block[texIndexIy*texture->width + texIndexIx];
                        float modifier[4] = {texCol.red, texCol.green, texCol.blue, texCol.alpha};
                        for (size_t i = 0; i < 4; i++) modifier[i] /= 255;

                        // Multiply the fragment color by the texel color modifier
                        rasterFragment.colour.red = rasterFragment.colour.red * modifier[0];
                        rasterFragment.colour.green = rasterFragment.colour.green * modifier[1];
                        rasterFragment.colour.blue = rasterFragment.colour.blue * modifier[2];
                        rasterFragment.colour.alpha = rasterFragment.colour.alpha * modifier[3];
                    }
                }
            }
            // Compute depth using barycentric interp
            rasterFragment.depth = alpha * vertex0.position.z + beta * vertex1.position.z + gamma * vertex2.position.z;

            // now we add it to the queue for fragment processing
            fragmentQueue.push_back(rasterFragment);
            } // per pixel
        } // per row

    } // RasteriseTriangle()

// process a single fragment
void FakeGL::ProcessFragment()
    { // ProcessFragment()
        // Get fragment from queue
        fragmentWithAttributes frag = fragmentQueue.front();
        fragmentQueue.pop_front();

        // If the fragment is too near or too far from the camera
        if (frag.depth < dNear or frag.depth > dFar) {
            // Stop processing the fragment
            return;
        }

        // If the fragment is in view of the camera horizontally and vertically
        if (frag.row >= 0 and frag.col >= 0 and frag.row < frameBuffer.height and frag.col < frameBuffer.width) {
            // Find the index into the framebuffer of the fragment
            size_t fragIndex = (frag.row * frameBuffer.width)+frag.col;

            if (dBufferingEnabled) {
                // Map from [dNear,dFar]->[0,255]

                // Find the fragment depth mapped from [0,255]
                unsigned char cFragDepth = (unsigned char)(((frag.depth - dNear)/(dFar-dNear))*255.);
                
                // If the fragment is behind an already drawn fragment
                if (cFragDepth > depthBuffer.block[fragIndex].alpha)
                    // Stop processing the fragment
                    return;
                else 
                    depthBuffer.block[fragIndex].alpha = cFragDepth;
            }

            // Draw the fragment to screen
            frameBuffer.block[fragIndex].red = frag.colour.red;
            frameBuffer.block[fragIndex].green = frag.colour.green;
            frameBuffer.block[fragIndex].blue = frag.colour.blue;
            frameBuffer.block[fragIndex].alpha = frag.colour.alpha;
        }
    } // ProcessFragment()


// routine for clamping a float value
float clamp(float v, float l, float h) {
    return std::min(std::max(v, l),h);
}

// standard routine for dumping the entire FakeGL context (except for texture / image)
std::ostream &operator << (std::ostream &outStream, FakeGL &fakeGL)
    { // operator <<
    outStream << "=========================" << std::endl;
    outStream << "Dumping FakeGL Context   " << std::endl;
    outStream << "=========================" << std::endl;

    outStream << "-------------------------" << std::endl;
    outStream << "Vertex Queue:            " << std::endl;
    outStream << "-------------------------" << std::endl;
    for (auto vertex = fakeGL.vertexQueue.begin(); vertex < fakeGL.vertexQueue.end(); vertex++)
        { // per matrix
        outStream << "Vertex " << vertex - fakeGL.vertexQueue.begin() << std::endl;
        outStream << *vertex;
        } // per matrix


    outStream << "-------------------------" << std::endl;
    outStream << "Raster Queue:            " << std::endl;
    outStream << "-------------------------" << std::endl;
    for (auto vertex = fakeGL.rasterQueue.begin(); vertex < fakeGL.rasterQueue.end(); vertex++)
        { // per matrix
        outStream << "Vertex " << vertex - fakeGL.rasterQueue.begin() << std::endl;
        outStream << *vertex;
        } // per matrix


    outStream << "-------------------------" << std::endl;
    outStream << "Fragment Queue:          " << std::endl;
    outStream << "-------------------------" << std::endl;
    for (auto fragment = fakeGL.fragmentQueue.begin(); fragment < fakeGL.fragmentQueue.end(); fragment++)
        { // per matrix
        outStream << "Fragment " << fragment - fakeGL.fragmentQueue.begin() << std::endl;
        outStream << *fragment;
        } // per matrix



    outStream << "-------------------------" << std::endl;
    outStream << "Projection Matrix:       " << std::endl;
    outStream << "-------------------------" << std::endl;
    outStream << fakeGL.pMatrixStack.top();

    outStream << "-------------------------" << std::endl;
    outStream << "Model/View Matrix:       " << std::endl;
    outStream << "-------------------------" << std::endl;
    outStream << fakeGL.mvMatrixStack.top();

    outStream << "-------------------------" << std::endl;
    outStream << "FakeGL State:            " << std::endl;
    outStream << "-------------------------" << std::endl;

    outStream << "fbClearColor " << fakeGL.fbClearColor << std::endl;
    outStream << "Depth Buffering: " << (fakeGL.dBufferingEnabled ? "On" : "Off") << std::endl;
    outStream << "Draw Color: " << fakeGL.drawColor << std::endl;
    outStream << "Window Position/Dimentions: X="<<fakeGL.windowX << " Y=" << fakeGL.windowY << 
                 " Width=" << fakeGL.windowWidth << " Height=" << fakeGL.windowHeight << std::endl;
    outStream << "Near plane: " << fakeGL.dNear << " Far plane: " << fakeGL.dFar << std::endl;
    outStream << "Normal: " << fakeGL.normal << std::endl;
    outStream << "Lighting: " << (fakeGL.lightingEnabled? "On":"Off") << std::endl;
    outStream << "Light Color:" <<std::endl;
    outStream << "  Ambient: ";
    for (size_t i = 0; i < 4; i++) outStream << fakeGL.lightAmbient[i] << " ";
    outStream << std::endl;
    outStream << "  Diffuse: ";
    for (size_t i = 0; i < 4; i++) outStream << fakeGL.lightDiffuse[i] << " ";
    outStream << std::endl;
    outStream << "  Specular: ";
    for (size_t i = 0; i < 4; i++) outStream << fakeGL.lightSpecular[i] << " ";
    outStream << std::endl;

    outStream << "Material Color:" <<std::endl;
    outStream << "  Ambient: ";
    for (size_t i = 0; i < 4; i++) outStream << fakeGL.matAmbient[i] << " ";
    outStream << std::endl;
    outStream << "  Diffuse: ";
    for (size_t i = 0; i < 4; i++) outStream << fakeGL.matDiffuse[i] << " ";
    outStream << std::endl;
    outStream << "  Specular: ";
    for (size_t i = 0; i < 4; i++) outStream << fakeGL.matSpecular[i] << " ";
    outStream << std::endl;
    outStream << "  Emission: ";
    for (size_t i = 0; i < 4; i++) outStream << fakeGL.matEmission[i] << " ";
    outStream << std::endl;
    outStream << "  Shininess: " << fakeGL.matShininess << std::endl;

    outStream << "Primitive Type: " << fakeGL.primType << std::endl;
    outStream << "Point Size: " << fakeGL.pointSize << std::endl;
    outStream << "Line Width: " << fakeGL.lineWidth << std::endl;

    outStream << "Textures: " << (fakeGL.textureEnabled? "On":"Off") << std::endl;
    outStream << "Texture Mode: " << fakeGL.texMode << std::endl;
    outStream << "Texture Coordinates: " << fakeGL.texCoord << std::endl;
    


    return outStream;
    } // operator <<

// subroutines for other classes
std::ostream &operator << (std::ostream &outStream, vertexWithAttributes &vertex)
    { // operator <<
    std::cout << "Vertex With Attributes" << std::endl;
    std::cout << "Position:   " << vertex.position << std::endl;
    std::cout << "Colour:     " << vertex.colour << std::endl;

	// you

    return outStream;
    } // operator <<

std::ostream &operator << (std::ostream &outStream, screenVertexWithAttributes &vertex) 
    { // operator <<
    std::cout << "Screen Vertex With Attributes" << std::endl;
    std::cout << "Position:   " << vertex.position << std::endl;
    std::cout << "Colour:     " << vertex.colour << std::endl;

    return outStream;
    } // operator <<

std::ostream &operator << (std::ostream &outStream, fragmentWithAttributes &fragment)
    { // operator <<
    std::cout << "Fragment With Attributes" << std::endl;
    std::cout << "Row:        " << fragment.row << std::endl;
    std::cout << "Col:        " << fragment.col << std::endl;
    std::cout << "Depth:      " << fragment.depth << std::endl;
    std::cout << "Colour:     " << fragment.colour << std::endl;

    return outStream;
    } // operator <<


    
    