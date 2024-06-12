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
 
#include "Raytracer.h"
#include <math.h>
#include <algorithm>

//-------------------------------------------------//
//                                                 //
// CONSTRUCTOR / DESTRUCTOR                        //
//                                                 //
//-------------------------------------------------//

// constructor
Raytracer::Raytracer()
    { // constructor
        // Initialise the Matrix stacks with empty matrices
        mvMatrixStack.push(Matrix4());
    } // constructor

// destructor
Raytracer::~Raytracer()
    { // destructor
    } // destructor

//-------------------------------------------------//
//                                                 //
// GEOMETRIC PRIMITIVE ROUTINES                    //
//                                                 //
//-------------------------------------------------//

// starts a sequence of geometric primitives
void Raytracer::Begin()
    { // Begin()
        // Clear the queues incase too many vertices were listed in the previous Begin & End call pair
        vertexQueue.clear();
    } // Begin()

// ends a sequence of geometric primitives
void Raytracer::End()
    { // End()
    // Convert Vertex queue to triangles
        while (vertexQueue.size() > 2)
        {
            eyeSpaceTriangle t;
            vertexVect.push_back(vertexQueue.front());
            t.v1 = &(vertexVect.back());
            vertexQueue.pop_front();

            vertexVect.push_back(vertexQueue.front());
            t.v2 = &(vertexVect.back());
            vertexQueue.pop_front();

            vertexVect.push_back(vertexQueue.front());
            t.v3 = &(vertexVect.back());
            vertexQueue.pop_front();

            t.textured = textureEnabled;

            t.impulse = t.v1->impulse;

            triangleVect.push_back(t);
        }
        
    } // End()

//-------------------------------------------------//
//                                                 //
// MATRIX MANIPULATION ROUTINES                    //
//                                                 //
//-------------------------------------------------//

// pushes a matrix on the stack
void Raytracer::PushMatrix()
    { // PushMatrix()
        // Copies the top of the stack and pushes it on
        mvMatrixStack.push(mvMatrixStack.top());
    } // PushMatrix()

// pops a matrix off the stack
void Raytracer::PopMatrix()
    { // PopMatrix()
        mvMatrixStack.pop();
        // If the pop results in an empty stack, fill with an empty matrix
        if (mvMatrixStack.size() == 0) {
            mvMatrixStack.push(Matrix4());
        }
    } // PopMatrix()

// load the identity matrix
void Raytracer::LoadIdentity()
    { // LoadIdentity()
        mvMatrixStack.top().SetIdentity();
    } // LoadIdentity()

// multiply by a known matrix in column-major format
void Raytracer::MultMatrixf(const float *columnMajorCoordinates)
    { // MultMatrixf()

        // Convert the matrix passed in to a Matrix4 instance
        Matrix4 inMat;
        for (size_t i = 0; i < 16; i++)
        {
            inMat[i%4][(size_t)floor(i/4)]=columnMajorCoordinates[i];
        }

        // Multiply the stack by said matrix
        mvMatrixStack.top() = mvMatrixStack.top() * inMat;
        
    } // MultMatrixf()

// sets up a perspective projection matrix
void Raytracer::Frustum(float left, float right, float bottom, float top, float zNear, float zFar)
    { // Frustum()
    } // Frustum()

// sets an orthographic projection matrix
void Raytracer::Ortho(float left, float right, float bottom, float top, float zNear, float zFar)
    { // Ortho()
    } // Ortho()

// rotate the matrix
void Raytracer::Rotatef(float angle, float axisX, float axisY, float axisZ)
    { // Rotatef()
        // Create the desired rotation as a matrix
        Matrix4 rotMat;
        rotMat.SetRotation(Cartesian3(axisX,axisY,axisZ),angle);

        mvMatrixStack.top() = mvMatrixStack.top() * rotMat;
    } // Rotatef()

// scale the matrix
void Raytracer::Scalef(float xScale, float yScale, float zScale)
    { // Scalef()
        Matrix4 scaleMat;
        scaleMat.SetScale(xScale,yScale,zScale);

        mvMatrixStack.top() = mvMatrixStack.top() * scaleMat;
    } // Scalef()

// translate the matrix
void Raytracer::Translatef(float xTranslate, float yTranslate, float zTranslate)
    { // Translatef()
        Matrix4 transMat;
        transMat.SetTranslation(Cartesian3(xTranslate,yTranslate,zTranslate));

        mvMatrixStack.top() = mvMatrixStack.top() * transMat;
    } // Translatef()

// sets the viewport
void Raytracer::Viewport(int x, int y, int width, int height)
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
void Raytracer::Color3f(float red, float green, float blue)
    { // Color3f()
        drawColor.red = red;
        drawColor.green = green;
        drawColor.blue = blue;
        drawColor.alpha = 1.f;
    } // Color3f()

// sets material properties
void Raytracer::Materialf(unsigned int parameterName, const float parameterValue)
    { // Materialf()
        if (parameterName & RT_SHININESS) {
            matCol.shininess = parameterValue;
        }
    } // Materialf()

void Raytracer::Materialfv(unsigned int parameterName, const float *parameterValues)
    { // Materialfv()

        if (parameterName & RT_AMBIENT_AND_DIFFUSE) {
            matCol.ambient = parameterValues;
            matCol.diffuse = parameterValues;
        }
        if (parameterName & RT_AMBIENT) {
            matCol.ambient = parameterValues;
        }
        if (parameterName & RT_DIFFUSE) {
            matCol.diffuse = parameterValues;
        }
        if (parameterName & RT_SPECULAR) {
            matCol.specular = parameterValues;
        }
        if (parameterName & RT_EMISSION) {
            matCol.emission = parameterValues;
        }
    } // Materialfv()

// sets the normal vector
void Raytracer::Normal3f(float x, float y, float z)
    { // Normal3f()
        normal = mvMatrixStack.top() * Cartesian3(x,y,z);
    } // Normal3f()

// sets the texture coordinates
void Raytracer::TexCoord2f(float u, float v)
    { // TexCoord2f()
    texCoord = Cartesian3(u,v,0.f);
    } // TexCoord2f()

// sets the vertex & launches it down the pipeline
void Raytracer::Vertex3f(float x, float y, float z)
    { // Vertex3f()
        // Generate a new vertex with the properties filled by the current state of the FakeGL instance
        vertexWithAttributes newVert;
        newVert.position = mvMatrixStack.top() * Cartesian3(x,y,z);
        newVert.colour.red = drawColor.red;
        newVert.colour.green = drawColor.green;
        newVert.colour.blue = drawColor.blue;
        newVert.colour.alpha = drawColor.alpha;
        newVert.normal = normal;
        newVert.impulse = impulseVert;

        newVert.material = matCol;

        newVert.texCoord = texCoord;

        // Add it to the vertex queue
        vertexQueue.push_back(newVert);
    } // Vertex3f()


void Raytracer::SetImpulse(bool state) {
    impulseVert = state;
}

//-------------------------------------------------//
//                                                 //
// STATE VARIABLE ROUTINES                         //
//                                                 //
//-------------------------------------------------//

// disables a specific flag in the library
void Raytracer::Disable(unsigned int property)
    { // Disable()
        if (property == RT_LIGHTING) {
            lightingEnabled = false;
        }
        if (property == RT_SHADOWS) {
            shadowsEnabled = false;
        }
        if (property == RT_TEXTURE_2D) {
            textureEnabled = false;
        }
        if (property == RT_IMPULSE_REFLECTION) {
            impulseEnabled = false;
        }
    } // Disable()

// enables a specific flag in the library
void Raytracer::Enable(unsigned int property)
    { // Enable()
        if (property == RT_LIGHTING) {
            lightingEnabled = true;
        }
        if (property == RT_SHADOWS) {
            shadowsEnabled = true;
        }
        if (property == RT_TEXTURE_2D) {
            textureEnabled = true;
        }
        if (property == RT_IMPULSE_REFLECTION) {
            impulseEnabled = true;
        }
    } // Enable()

//-------------------------------------------------//
//                                                 //
// LIGHTING STATE ROUTINES                         //
//                                                 //
//-------------------------------------------------//

// sets properties for the one and only light
void Raytracer::Light(int parameterName, const float *parameterValues)
    { // Light()
        if (parameterName & RT_POSITION) {
            lightPosition = Homogeneous4(parameterValues[0],
                                         parameterValues[1],
                                         parameterValues[2],
                                         parameterValues[3]);

            // Transform the light position by the Model View matrix so that the position is stored in Eye space (VCS)
            lightPosition = mvMatrixStack.top() * lightPosition;
        }
        if (parameterName & RT_AMBIENT) {
            lightMat.ambient = parameterValues;

            // For some reason offsetting the ambient by .2 seems to make the lighting look closer to openGL
            lightMat.ambient += MatComponent(0.2f,0.2f,0.2f,0.f);
        }
        if (parameterName & RT_DIFFUSE) {
            lightMat.diffuse = parameterValues;
        }
        if (parameterName & RT_SPECULAR) {
            lightMat.specular = parameterValues;
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
void Raytracer::TexEnvMode(unsigned int textureMode)
    { // TexEnvMode()
    texMode = textureMode;
    } // TexEnvMode()

// sets the texture image that corresponds to a given ID
void Raytracer::TexImage2D(const RGBAImage &textureImage)
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
void Raytracer::Clear(unsigned int mask)
    { // Clear()        

        // If clear color buffer is set
        if (mask & RT_COLOR_BUFFER_BIT) {
            for (size_t i = 0; i < frameBuffer.height*frameBuffer.width; i++)
            {
                // Update every pixel to be the clear color
                frameBuffer.block[i] = fbClearColor.toRGBAValue();
            }
        }
        // If clear depth buffer is set
        if (mask & RT_DEPTH_BUFFER_BIT) {
            // Reset depth buffer to max value
            for (size_t i = 0; i < depthBuffer.width*depthBuffer.height; i++)
                depthBuffer.block[i].alpha = 255;
        }
        vertexVect.clear();
        triangleVect.clear();
    } // Clear()

// sets the clear colour for the frame buffer
void Raytracer::ClearColor(float red, float green, float blue, float alpha)
    { // ClearColor()
        fbClearColor.red = red;
        fbClearColor.green = green;
        fbClearColor.blue = blue;
        fbClearColor.alpha = alpha;
    } // ClearColor()

//-------------------------------------------------//
//                                                 //
// MAJOR PROCESSING ROUTINES                       //
//                                                 //
//-------------------------------------------------//

// casts a ray in the world and returns pointer to the first triangle it interescts
// Null pointer if no triangle intersection
surfel Raytracer::RayCast(ray r) {
    float minD = 999999.f;
    surfel closest;
    surfel intersec(NULL,0,0,0,0);
    for (size_t i = 0; i < triangleVect.size(); i++)
    {
        if (RayTriIntersectTest(r, &(triangleVect[i]), intersec)) {
            if (intersec.distance >= 0 and intersec.distance < minD) {
                if (intersec.tri->impulse and impulseEnabled) {
                    ray r1;
                    r1.dir = reflectVector(r.dir, intersec.getNorm());
                    r1.origin = intersec.getPos() + 0.001*r1.dir;

                    float oldD = intersec.distance;

                    intersec = RayCast(r1);
                    intersec.distance = oldD;
                }
                minD = intersec.distance;
                closest = intersec;
            }
        }
    }
    return closest;
}

// casts a ray in the world and returns distance to triangle tri
surfel Raytracer::RayCast(ray r, eyeSpaceTriangle *tri) {
    surfel intersec(NULL,0,0,0,0);
    // If the ray and triangle intersect
    if (RayTriIntersectTest(r, tri, intersec)) {
        // If the triangle is infront of the camera
        if (intersec.distance >= 0) {
            // If the intersected tri is an impulse reflection
            // std::cout << "tri: " << intersec.tri->impulse << " rt impulse: " << impulseEnabled << std::endl;
            if (intersec.tri->impulse and impulseEnabled) {
                ray r1;
                r1.dir = reflectVector(r.dir, intersec.getNorm());
                r1.origin = intersec.getPos() + 0.001*r1.dir;

                float oldD = intersec.distance;

                intersec = RayCast(r1);
                intersec.distance = oldD;
            }
            return intersec;
        }
    }
    return surfel();
}


bool Raytracer::RayTriIntersectTest(ray r, eyeSpaceTriangle* tri, surfel &intersec) {
    // Find the plane that the triangle lies on
    Cartesian3 AB = tri->v2->position - tri->v1->position;
    Cartesian3 AC = tri->v3->position - tri->v1->position;
    Cartesian3 N = AB.cross(AC);

    // Find the t value for the intersect of r and the plane of tri where r = O + tD
    Cartesian3 w = tri->v1->position - r.origin;
    float distance = w.dot(N) / r.dir.dot(N);

    // Where the ray intersects the plane
    Cartesian3 planeIntersect = r.origin + (r.dir * distance);

    float alpha, beta, gamma;
    getBarycentric(planeIntersect, tri->v1->position, tri->v2->position,tri->v3->position, alpha, beta, gamma);

    // If point is inside the triangle
    if (alpha >= 0.f and alpha <= 1.f and beta >= 0.f and beta <= 1.f and gamma >= 0.f and gamma <= 1.f) {
        intersec = surfel(tri, alpha, beta, gamma, distance);
        return true;
    }
    return false;
}

// Caching triangle values would make this far more efficient
void Raytracer::getBarycentric(Cartesian3 p, Cartesian3 a, Cartesian3 b, Cartesian3 c, float &alpha, float &beta, float &gamma) {
    Cartesian3 v0 = b-a, v1 = c-a, v2 = p-a;

    float dot00 = v0.dot(v0),
          dot01 = v0.dot(v1),
          dot11 = v1.dot(v1),
          dot20 = v2.dot(v0),
          dot21 = v2.dot(v1);

    float invDenominator = 1.f / ((dot00 * dot11) - (dot01 * dot01));

    beta = ((dot11*dot20)-(dot01*dot21)) * invDenominator;
    gamma = ((dot00*dot21) - (dot01 * dot20)) * invDenominator;
    alpha = 1.f - beta - gamma;
}

// Draw screen by looping triangles then pixels
void Raytracer::drawScreenByTri() {
    float pWidth = 2.f/(float)frameBuffer.width;
    float pHeight = 2.f/(float)frameBuffer.height;

    ray r;
    r.dir = Cartesian3(0.f,0.f,-1.f);

    for (size_t i = 0; i < triangleVect.size(); i++) 
    {
        eyeSpaceTriangle *tri = &(triangleVect[i]);

        // For each pixel
        for (size_t x = 0; x < frameBuffer.width; x++)
        {
            float xOff = -1.f + (x*pWidth) + (pWidth/2.);
            for (size_t y = 0; y < frameBuffer.height; y++)
            {
                float yOff = -1.f + (y*pHeight) + (pHeight/2.);

                // Cast the orthogonal ray
                r.origin = Cartesian3(xOff,yOff,1.f);
                surfel intersec = RayCast(r, tri);

                // If the ray intersects a triangle
                if (intersec.tri != NULL) {
                    if ((intersec.distance*128.f) > depthBuffer[y][x].alpha) {
                        continue;
                    }
                    depthBuffer[y][x].alpha = (intersec.distance*128.f);
                    RenderIntersec(intersec, x, y);
                }
            }
        }
    }
    
}

// Draw screen by looping pixels then triangles
void Raytracer::drawScreenByPix() {
    float pWidth = 2.f/(float)frameBuffer.width;
    float pHeight = 2.f/(float)frameBuffer.height;

    ray r;
    r.dir = Cartesian3(0.f,0.f,-1.f);

    // For each pixel
    for (size_t x = 0; x < frameBuffer.width; x++)
    {
        float xOff = -1.f + (x*pWidth) + (pWidth/2.);
        for (size_t y = 0; y < frameBuffer.height; y++)
        {
            float yOff = -1.f + (y*pHeight) + (pHeight/2.);

            // Cast the orthogonal ray
            r.origin = Cartesian3(xOff,yOff,1.f);
            surfel intersec = RayCast(r);

            // If the ray intersects a triangle
            if (intersec.tri != NULL) {
                RenderIntersec(intersec, x, y);
            }
        }
    }
}

void Raytracer::RenderIntersec(surfel inter, size_t x, size_t y) {
    // Get pointers to the vertices for readable code
    vertexWithAttributes *v1 = inter.tri->v1;
    vertexWithAttributes *v2 = inter.tri->v2;
    vertexWithAttributes *v3 = inter.tri->v3;

    FRGBAValue pixColour = FRGBAValue(0.f,0.f,0.f,1.f);

    if (lightingEnabled) {
        // Interpolate the position, normal, and material
        Cartesian3 pixPos = (v1->position * inter.alpha) + (v2->position * inter.beta) + (v3->position * inter.gamma);
        Cartesian3 pixNormal = (v1->normal * inter.alpha )+ (v2->normal * inter.beta) + (v3->normal * inter.gamma);
        pixNormal=pixNormal.unit();
        Material pixMat = (v1->material * inter.alpha) + (v2->material * inter.beta) + (v3->material * inter.gamma);

        MatComponent totalLight(0.f,0.f,0.f,0.f);
        
        // Ambient and emiisive lighting
        totalLight += pixMat.emission;
        totalLight += (pixMat.ambient * lightMat.ambient);

        Cartesian3 lightDir = lightPosition.Vector().unit();
        Cartesian3 viewDir = pixPos.unit();

        // Check shadows
        ray r;
        r.dir = lightDir;
        r.origin = pixPos + (0.01*lightDir.unit());

        // If there is nothing blocking the light source
        if (RayCast(r).tri == NULL or !shadowsEnabled) {
            // Diffuse lighting
            float diffuse = std::max(pixNormal.dot(lightDir), 0.0f);
            totalLight += (pixMat.diffuse * lightMat.diffuse * diffuse);

            // Specular lighting
            Cartesian3 bisector = (lightDir+viewDir).unit();
            float specular = std::pow(std::max(pixNormal.dot(bisector),0.0f),pixMat.shininess);
            totalLight += (pixMat.specular * lightMat.specular * specular);
        }

        // Alpha is only determined by diffuse of the object
        totalLight[3] = pixMat.diffuse[3];

        // Set vertex color to the lighting color, limiting to range [0,255]
        pixColour.red = totalLight[0];
        pixColour.green = totalLight[1];
        pixColour.blue = totalLight[2];
        pixColour.alpha = totalLight[3];
    } else {
        // Set the pixel to the barycentric interpolated colour of the triangle
        pixColour = (inter.alpha * v1->colour) + 
                    (inter.beta * v2->colour) +
                    (inter.gamma * v3->colour);
    }

    // std::cout<<"tex=" <<textureEnabled<<std::endl;
    if (inter.tri->textured) {
        RGBAValue pixTexture;

        // Barycentric interp texture coordinates
        Cartesian3 pixTexCoords = (v1->texCoord * inter.alpha) + (v2->texCoord * inter.beta) + (v3->texCoord * inter.gamma);

        // Convert to texel coordinates
        size_t texIndexIx = (size_t)(pixTexCoords.x * texture->width); 
        size_t texIndexIy = (size_t)(pixTexCoords.y * texture->height); 

        // This is to prevent a rare segmentation fault that  I think is caused by an attempt to access a texel outside fo the textures range
        if (texIndexIx < texture->width and texIndexIx >= 0 and texIndexIy < texture->height and texIndexIy >= 0) {
            pixTexture = texture->block[texIndexIy*texture->width + texIndexIx];
        }

        // Handle blending mode
        if (texMode == RT_REPLACE) {
            pixColour = pixTexture;
        } else if (texMode == RT_MODULATE) {
            float modifier[4] = {pixTexture.red, pixTexture.green, pixTexture.blue, pixTexture.alpha};
            for (size_t i = 0; i < 4; i++) modifier[i] /= 255.f;

            pixColour.red *= modifier[0];
            pixColour.green *= modifier[1];
            pixColour.blue *= modifier[2];
            pixColour.alpha *= modifier[3];
        }
    }
    frameBuffer[y][x] = pixColour.toRGBAValue();
}

// routine for clamping a float value
float clamp(float v, float l, float h) {
    return std::min(std::max(v, l),h);
}

// Reflects a vector, v on a normal n
Cartesian3 reflectVector(Cartesian3 v, Cartesian3 n) {
    Cartesian3 uN = n.unit();
    float dDotN = v.dot(uN);
    return v - (2*dDotN*uN);
}
