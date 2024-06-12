//////////////////////////////////////////////////////////////////////
//
//  University of Leeds
//  COMP 5812M Foundations of Modelling & Rendering
//  User Interface for Coursework
//
//  September, 2020
//
//  -----------------------------
//  Raytrace Render Widget
//  -----------------------------
//
//	Provides a widget that displays a fixed image
//	Assumes that the image will be edited (somehow) when Render() is called
//  
////////////////////////////////////////////////////////////////////////

#include <math.h>

// include the header file
#include "RaytraceRenderWidget.h"

// constructor
RaytraceRenderWidget::RaytraceRenderWidget
        (   
        // the geometric object to show
        TexturedObject      *newTexturedObject,
        // the render parameters to use
        RenderParameters    *newRenderParameters,
        // parent widget in visual hierarchy
        QWidget             *parent
        )
    // the : indicates variable instantiation rather than arbitrary code
    // it is considered good style to use it where possible
    : 
    // start by calling inherited constructor with parent widget's pointer
    QOpenGLWidget(parent),
    // then store the pointers that were passed in
    texturedObject(newTexturedObject),
    renderParameters(newRenderParameters)
    { // constructor
    // leaves nothing to put into the constructor body

    } // constructor    

// destructor
RaytraceRenderWidget::~RaytraceRenderWidget()
    { // destructor
    // empty (for now)
    // all of our pointers are to data owned by another class
    // so we have no responsibility for destruction
    // and OpenGL cleanup is taken care of by Qt
    } // destructor                                                                 

// called when OpenGL context is set up
void RaytraceRenderWidget::initializeGL()
    { // RaytraceRenderWidget::initializeGL()
	// this should remain empty

    texturedObject->TransferAssetsToRaytracer(&raytracer);
    } // RaytraceRenderWidget::initializeGL()

// called every time the widget is resized
void RaytraceRenderWidget::resizeGL(int w, int h)
    { // RaytraceRenderWidget::resizeGL()
    // resize the render image
    raytracer.frameBuffer.Resize(w, h);
    raytracer.depthBuffer.Resize(w, h);
    } // RaytraceRenderWidget::resizeGL()
    
// called every time the widget needs painting
void RaytraceRenderWidget::paintGL()
    { // RaytraceRenderWidget::paintGL()
    // set background colour to white

    Raytrace();


    // and display the image
    glDrawPixels(raytracer.frameBuffer.width, raytracer.frameBuffer.height, GL_RGBA, GL_UNSIGNED_BYTE, raytracer.frameBuffer.block);
    } // RaytraceRenderWidget::paintGL()
    
    // routine that generates the image
void RaytraceRenderWidget::Raytrace()
    { // RaytraceRenderWidget::Raytrace()
	// This is where you will invoke your raytracing

    raytracer.ClearColor(0.8, 0.8, 0.6, 1.0);
    raytracer.Clear(RT_COLOR_BUFFER_BIT | RT_DEPTH_BUFFER_BIT);
    raytracer.LoadIdentity();

    raytracer.Disable(RT_LIGHTING);
    raytracer.Disable(RT_SHADOWS);
    raytracer.Disable(RT_IMPULSE_REFLECTION);

    if (renderParameters->shadowsOn) {
        raytracer.Enable(RT_SHADOWS);
    }

    if (renderParameters->impulseReflectionOn) {
        raytracer.Enable(RT_IMPULSE_REFLECTION);
    }

    // if lighting is turned on
    if (renderParameters->useLighting)
    { // use lighting
        // make sure lighting is on
        raytracer.Enable(RT_LIGHTING);

        // set light position first, pushing/popping matrix so that it the transformation does
        // not affect the position of the geometric object
        raytracer.PushMatrix();
        raytracer.MultMatrixf(renderParameters->lightMatrix.columnMajor().coordinates);
        raytracer.Light(RT_POSITION, renderParameters->lightPosition);
        raytracer.PopMatrix();
        
        // now set the lighting parameters (assuming all light is white)
        float ambientColour[4];
        float diffuseColour[4];
        float specularColour[4];
        
        // now copy the parameters
        ambientColour[0]    = ambientColour[1]  = ambientColour[2]  = renderParameters->ambientLight;
        diffuseColour[0]    = diffuseColour[1]  = diffuseColour[2]  = renderParameters->diffuseLight;
        specularColour[0]   = specularColour[1] = specularColour[2] = renderParameters->specularLight;
        ambientColour[3]    = diffuseColour[3]  = specularColour[3] = 1.0; // don't forget alpha

        // and set them in OpenGL
        raytracer.Light(RT_AMBIENT,    ambientColour);
        raytracer.Light(RT_DIFFUSE,    diffuseColour);
        raytracer.Light(RT_SPECULAR,   specularColour);
    }

    // translate by the visual translation
    raytracer.Translatef(renderParameters->xTranslate, renderParameters->yTranslate, 0.0f);

    // apply rotation matrix from arcball
    raytracer.MultMatrixf(renderParameters->rotationMatrix.columnMajor().coordinates);

    if (renderParameters->showObject) {
        texturedObject->RenderRT(renderParameters,&raytracer);
    }

    if (renderParameters->renderRT) {
        raytracer.drawScreenByTri();
    }


    } // RaytraceRenderWidget::Raytrace()
    
// mouse-handling
void RaytraceRenderWidget::mousePressEvent(QMouseEvent *event)
    { // RaytraceRenderWidget::mousePressEvent()
    // store the button for future reference
    int whichButton = event->button();
    // scale the event to the nominal unit sphere in the widget:
    // find the minimum of height & width   
    float size = (width() > height()) ? height() : width();
    // scale both coordinates from that
    float x = (2.0 * event->x() - size) / size;
    float y = (size - 2.0 * event->y() ) / size;

    
    // and we want to force mouse buttons to allow shift-click to be the same as right-click
    int modifiers = event->modifiers();
    
    // shift-click (any) counts as right click
    if (modifiers & Qt::ShiftModifier)
        whichButton = Qt::RightButton;
    
    // send signal to the controller for detailed processing
    emit BeginScaledDrag(whichButton, x,y);
    } // RaytraceRenderWidget::mousePressEvent()
    
void RaytraceRenderWidget::mouseMoveEvent(QMouseEvent *event)
    { // RaytraceRenderWidget::mouseMoveEvent()
    // scale the event to the nominal unit sphere in the widget:
    // find the minimum of height & width   
    float size = (width() > height()) ? height() : width();
    // scale both coordinates from that
    float x = (2.0 * event->x() - size) / size;
    float y = (size - 2.0 * event->y() ) / size;
    
    // send signal to the controller for detailed processing
    emit ContinueScaledDrag(x,y);
    } // RaytraceRenderWidget::mouseMoveEvent()
    
void RaytraceRenderWidget::mouseReleaseEvent(QMouseEvent *event)
    { // RaytraceRenderWidget::mouseReleaseEvent()
    // scale the event to the nominal unit sphere in the widget:
    // find the minimum of height & width   
    float size = (width() > height()) ? height() : width();
    // scale both coordinates from that
    float x = (2.0 * event->x() - size) / size;
    float y = (size - 2.0 * event->y() ) / size;
    
    // send signal to the controller for detailed processing
    emit EndScaledDrag(x,y);
    } // RaytraceRenderWidget::mouseReleaseEvent()
