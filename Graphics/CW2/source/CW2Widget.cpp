#include <GL/glu.h>
#include <GL/glut.h>
#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <math.h>
#include <cmath>
#include <cstdlib>
#include "CW2Widget.h"
#include "CW2Materials.h"
#include "CW2PPMImg.h"
#include "CW2MatWindow.h"

#include <QGLWidget>
#include <QApplication>
#include <QTimer>
#include <QMouseEvent>
#include <QPoint>
#include <QString>

// ----Init and Open GL Functions----
// ----------------------------------

// Constructor Function
CW2Widget::CW2Widget(QWidget *parent) : QGLWidget(parent) {
	// So that the widget can be focused on and receive inputs
	setFocusPolicy(Qt::StrongFocus);

	// Camera Position in coordinates
	// (x,y,z)
	cameraPos[0] = -2.5;
	cameraPos[1] = 1.9;
	cameraPos[2] = -2.5;
	// Camera angle in degrees
	// (yaw, pitch down)
	cameraDir[0] = 45.;
	cameraDir[1] = 0.;
	// Current Velocity of the camera
	// (x,y,z)
	cameraVel[0] = 0.;
	cameraVel[1] = 0.;
	cameraVel[2] = 0.;

	// The angle of each arm of the robot
	robotAngle[0] = 0;
	robotAngle[1] = 45;

	// How may times a second the scene will update
	frameRate = 60;

	mousePressed = false;

	// Variable to store whether the robot should be spinning automatically, or user controlled
	robotAuto = true;

	// Initialise quadric objects
	roboBase = gluNewQuadric();
	roboBaseTop = gluNewQuadric();
	roboArm = gluNewQuadric();
	roboArmTop = gluNewQuadric();
	roboArmBottom = gluNewQuadric();
	roboTorchHandle = gluNewQuadric();
	roboTorchHandleBottom = gluNewQuadric();
	roboTorchFlame = gluNewQuadric();

	// The 'material' for the torch light
	torchLightMat = {
    { 2, 1, 0, 1. },
    { 2, 1, 0, 1. },
    { 2, 1, 0, 1. },
    0.
	};

	// An intensity modifier for the torch light
	// Used to create flickering effect
	torchModifier = 1;

	// initialise the material window
	matWindow = new CW2MatWindow(NULL);
	// Connect the callback returning the input data from the matWindow to this widget
	connect(matWindow,&CW2MatWindow::retMaterial, this, &CW2Widget::updateLight);

	// Set up a timer Signal/Slot to update the program at the desired framerate
	QTimer *timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(updateGL()));
	int frameTime = (int)(1000 / frameRate);
	timer->start(frameTime);
}

// Destructor Function
CW2Widget::~CW2Widget() {
	// Clean up quadric objects
	gluDeleteQuadric(roboBase);
	gluDeleteQuadric(roboBaseTop);
	gluDeleteQuadric(roboArm);
	gluDeleteQuadric(roboArmTop);
	gluDeleteQuadric(roboArmBottom);
	gluDeleteQuadric(roboTorchHandle);
	gluDeleteQuadric(roboTorchHandleBottom);
	gluDeleteQuadric(roboTorchFlame);

	// Clean up the mat window
	delete matWindow;

	// Clean up the textures used
	glDeleteTextures(4,textures);
}

// Called once when openGl is initialized
void CW2Widget::initializeGL() {
	// Clear the window to black
	glClearColor(0, 0, 0, 1.);

	// Store all of the texture paths
	uint tCount = 4;
	std::string texPaths[] = {"textures/earth.ppm",
							  "textures/Moi.ppm",
							  "textures/brick.ppm",
							  "textures/wood.ppm"};

	// Enable textures and create however many textures are needed
	glEnable(GL_TEXTURE_2D);
	glGenTextures(tCount,textures);

	// For each texture path
	for (uint i = 0; i < tCount; i++) {
		// Load that path as an image
		CW2PPMImg* ppm = new CW2PPMImg(texPaths[i]);

		// Load the corresponding index of 'textures' into openGL
		glBindTexture(GL_TEXTURE_2D, textures[i]);
		// Initialise the texutre with the data from the PPM image
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, ppm->W(),ppm->H(),0,GL_RGB,GL_UNSIGNED_BYTE,ppm->Data());
		// Set perameters for the texture
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		// Clean up the image object
		delete ppm;
	}
}

// Called whenever the widget is resized
void CW2Widget::resizeGL(int w, int h) {
	glViewport(0, 0, w, h);

	// Set the modelview matrix to the identity matrix
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	// Enable lighting in openGL
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	// Position the light
	GLfloat lightPos[] = {0.,0.,0.,0.};
	glLightfv(GL_LIGHT0,GL_POSITION, lightPos);

	// Set the projection matrix to the identity matrix
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	// Calculate the aspect ratio of the window
	GLdouble aspect = (double)width() / (double)height();
	// Set the projection to perspective mode
	gluPerspective(90,aspect,0.1,40);

}

// Called whenever the widget is updated
void CW2Widget::paintGL() {
	// Clear the window
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Set the shading to be smooth on primatives rendered
	glShadeModel(GL_SMOOTH);


	// Setup the depth testing
	glEnable(GL_DEPTH_TEST);

	// Enable normal normalising
	glEnable(GL_NORMALIZE);

	// Draw the scene
	glPushMatrix();

	room();
	table();

	glPushMatrix();
	// 0, 1.1, 0
	glTranslatef(0,1.1,0);
	robot();

	glPopMatrix();

	glPopMatrix();

	// Switch to Model View for camera placement
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Calculate a point for the camera to look at that is in the direction fo the desired camera position and angle
	GLfloat cameraTarg[] = {cameraPos[0],cameraPos[1], cameraPos[2]};
	GLfloat o = sin(degToRad(cameraDir[1]));
	GLfloat a = cos(degToRad(cameraDir[1]));
	cameraTarg[0] += a * cos(degToRad(cameraDir[0]));
	cameraTarg[2] += a * sin(degToRad(cameraDir[0]));
	cameraTarg[1] += o;
	// Place the camera in the screen, at the position and looking at the point calculted
	gluLookAt(cameraPos[0],cameraPos[1],cameraPos[2], cameraTarg[0],cameraTarg[1],cameraTarg[2], 0.0, 1.0, 0.0);

	// Flush to screen
	glFlush();
}


// ---------Update Functions---------
// ----------------------------------

// Update the widget
void CW2Widget::updateGL() {
	// Handle the user input and update the camera position
	handleKeys();
	updateCamera();

	// Call parent class updateGL()
	QGLWidget::updateGL();
}

// Function to handle the keyboard input
// Called once per itteration of 'updateGL()'
void CW2Widget::handleKeys() {
	// Prosses Camera Movement Inputs
	GLfloat velVector[] = {0.,0.,0.};
	if (keysPressed[Qt::Key_W]) {
		velVector[0] += cos(degToRad(cameraDir[0]));
		velVector[2] += sin(degToRad(cameraDir[0]));
	}
	if (keysPressed[Qt::Key_S]) {
		velVector[0] -= cos(degToRad(cameraDir[0]));
		velVector[2] -= sin(degToRad(cameraDir[0]));
	}
	if (keysPressed[Qt::Key_A]) {
		velVector[0] -= cos(degToRad(cameraDir[0]+90));
		velVector[2] -= sin(degToRad(cameraDir[0]+90));
	}
	if (keysPressed[Qt::Key_D]) {
		velVector[0] += cos(degToRad(cameraDir[0]+90));
		velVector[2] += sin(degToRad(cameraDir[0]+90));
	}
	cameraVel[0] = velVector[0];
	cameraVel[1] = velVector[1];
	cameraVel[2] = velVector[2];

	if (!robotAuto) {
		// Process Robot Movement Inputs
		if (keysPressed[Qt::Key_Right]) {
			robotAngle[0] = fmod((float)robotAngle[0] - 1,360);
		}
		if (keysPressed[Qt::Key_Left]) {
			robotAngle[0] = fmod((float)robotAngle[0] + 1, 360);
		}
		if (keysPressed[Qt::Key_Up]) {
			robotAngle[1]++;
		}
		if (keysPressed[Qt::Key_Down]) {
			robotAngle[1]--;
		}
	} else {
		robotAngle[0] = fmod((float)robotAngle[0] - 1,360);
	}

	// Cap the robots angles between -10 and 190
	if (robotAngle[1] < -10) {
		robotAngle[1] = -10;
	} else if (robotAngle[1] > 190) {
		robotAngle[1] = 190;
	}
}

// Function for updating the camera positioning
void CW2Widget::updateCamera() {
	// Update the camera position according to the velocity
	GLfloat speed = 0.1;
	cameraPos[0] += cameraVel[0] * speed;
	cameraPos[1] += cameraVel[1] * speed;
	cameraPos[2] += cameraVel[2] * speed;

	// Ensure the camera stays inside the room
	if (cameraPos[0] > 2.8) {
		cameraPos[0] = 2.8;
	} else if (cameraPos[0] < -2.8) {
		cameraPos[0] = -2.8;
	}
	if (cameraPos[2] > 2.8) {
		cameraPos[2] = 2.8;
	} else if (cameraPos[2] < -2.8) {
		cameraPos[2] = -2.8;
	}
}

// Update the torch light material with the values returned by the user input
void CW2Widget::updateLight(GLfloat vals[3][4], GLfloat shin) {
	torchLightMat.ambient[0] = vals[0][0];
	torchLightMat.ambient[1] = vals[0][1];
	torchLightMat.ambient[2] = vals[0][2];
	torchLightMat.ambient[3] = vals[0][3];
	torchLightMat.diffuse[0] = vals[1][0];
	torchLightMat.diffuse[1] = vals[1][1];
	torchLightMat.diffuse[2] = vals[1][2];
	torchLightMat.diffuse[3] = vals[1][3];
	torchLightMat.specular[0] = vals[2][0];
	torchLightMat.specular[1] = vals[2][1];
	torchLightMat.specular[2] = vals[2][2];
	torchLightMat.specular[3] = vals[2][3];
	torchLightMat.shininess = shin;
}

// --------------Events--------------
// ----------------------------------

// Triggered when a key is first pressed
void CW2Widget::keyPressEvent(QKeyEvent *event) {
	// Find the key in question
	int key = event->key();
	// Escape and M are special cases that require immediate action
	// Escape = Close Application
	if (key == Qt::Key_Escape) {
		qApp->quit();
	// M = Open material input menu
	} else if (key == Qt::Key_M) {
		matWindow->show(&torchLightMat);
	// All other keys are stored in a hash map when they are pressed
	// All currently pressed keys are handles in "handleKeys()"
	// This allows for keys to be held down
	} else if (key == Qt::Key_N) {
		robotAuto = !robotAuto;
	} else {
		keysPressed[key] = true;
	}
}

// Triggered when a key is released
void CW2Widget::keyReleaseEvent(QKeyEvent *event) {
	int key = event->key();
	// Set it to unpressed in the dictionary
	keysPressed[key] = false;
}

// Triggered on a mouse click
void CW2Widget::mousePressEvent(QMouseEvent *event) {
	// Find the mouse position
	QPointF mPos = event->localPos();
	// If click was over the window
	if (mPos.x() >= 0 && mPos.x() <= width() && mPos.y() >= 0 && mPos.y() <= height()) {
		mousePressed = true;
		hideMouse();
	}
}

// Triggered when the mouse click is released
void CW2Widget::mouseReleaseEvent(QMouseEvent*) {
	mousePressed = false;
	showMouse();
}

// Triggered when the mouse is moved
void CW2Widget::mouseMoveEvent(QMouseEvent *event) {
	// If the mouse click is being held
	if (mousePressed) {
		// Find the middle of the window in coordinates relative to the window
		int middleW = (int) (width() / 2);
		int middleH = (int) (height() / 2);

		// Find mouse pos
		QPoint mPos = event->pos();

		// Find distance of the mouse from the middle of the window
		GLfloat deltaX = mPos.x() - middleW;
		GLfloat deltaY = mPos.y() - middleH;

		// Set Mouse sensitivity
		GLfloat sens = 0.05;

		// Spin the camera by an amount relative to how far from the middle the cursor is
		cameraDir[0] = std::fmod((cameraDir[0] + (deltaX* sens)), 360.);
		cameraDir[1] = cameraDir[1] - (deltaY* sens);
		// Cap the vertical angle between -89 and 89
		if (cameraDir[1] < -89) {
			cameraDir[1] = -89;
		} else if (cameraDir[1] > 89) {
			cameraDir[1] = 89;
		}

		// Reset the cursor to the middle
		hideMouse();
	} 
}

// ---------Drawable Objects---------
// ----------------------------------

// Function to draw a rectangularly tesselated rectangle with corner coordinates 'coordinates'', on the plane dimention 'plane'.
// Cornes must be the 2D coordinates of the rectangle, given in the order: bottom left, top right.
void CW2Widget::plane(GLfloat corners[2][2], int plane, GLfloat depth, GLfloat resolution) {
	GLfloat xStepSize = abs(corners[1][0] - corners[0][0]) / resolution;
	GLfloat yStepSize = abs(corners[1][1] - corners[0][1]) / resolution;

	// In x plane
	if (plane == 0) {
		glBegin(GL_QUADS);
		// Draws many planes that will make up one large, tesselated, plane
		for (GLfloat i = 0; i < 1; i += (1.0/resolution)) {
			for (GLfloat j = 0; j < 1; j += (1.0/resolution)) {
				GLfloat x = mapVal(i, 0, 1, corners[0][0], corners[1][0]);
				GLfloat y = mapVal(j, 0, 1, corners[0][1], corners[1][1]);
		 		glVertex3f(depth,x,y);
				glVertex3f(depth,x,y+yStepSize);
				glVertex3f(depth,x+xStepSize,y+yStepSize);
				glVertex3f(depth,x+xStepSize,y);
			}
		}
		
		glEnd();
	// In y plane
	} else if (plane == 1) {
		glBegin(GL_QUADS);
		for (GLfloat i = 0; i < 1; i += (1.0/resolution)) {
			for (GLfloat j = 0; j < 1; j += (1.0/resolution)) {
				GLfloat x = mapVal(i, 0, 1, corners[0][0], corners[1][0]);
				GLfloat y = mapVal(j, 0, 1, corners[0][1], corners[1][1]);
		 		glVertex3f(x,depth,y);
				glVertex3f(x,depth,y+yStepSize);
				glVertex3f(x+xStepSize,depth,y+yStepSize);
				glVertex3f(x+xStepSize,depth,y);
			}
		}
		glEnd();
	// In z plane
	} else if (plane == 2) {
		glBegin(GL_QUADS);
		for (GLfloat i = 0; i < 1; i += (1.0/resolution)) {
			for (GLfloat j = 0; j < 1; j += (1.0/resolution)) {
				GLfloat x = mapVal(i, 0, 1, corners[0][0], corners[1][0]);
				GLfloat y = mapVal(j, 0, 1, corners[0][1], corners[1][1]);
		 		glVertex3f(y,x,depth);
				glVertex3f(y+yStepSize,x,depth);
				glVertex3f(y+yStepSize,x+xStepSize, depth);
				glVertex3f(y, x+xStepSize,depth);
			}
		}
		glEnd();
	}
	
}

// This functions like 'plane()' however the resulting plane will be textured with the texture at the index 'tex_i' in the textures array
void CW2Widget::texturedPlane(GLfloat corners[2][2], int plane, GLfloat depth, GLfloat resolution, int tex_i) {
	GLfloat xStepSize = abs(corners[1][0] - corners[0][0]) / resolution;
	GLfloat yStepSize = abs(corners[1][1] - corners[0][1]) / resolution;
	GLfloat stepSize = 1.0 / resolution;

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, textures[tex_i]);

	// In x plane
	if (plane == 0) {
		glBegin(GL_QUADS);
		for (GLfloat i = 0; i < 1; i += (1.0/resolution)) {
			for (GLfloat j = 0; j < 1; j += (1.0/resolution)) {
				GLfloat x = mapVal(i, 0, 1, corners[0][0], corners[1][0]);
				GLfloat y = mapVal(j, 0, 1, corners[0][1], corners[1][1]);
				GLfloat texX =i + stepSize;
				GLfloat texY = 1.0 - j;
				glTexCoord2f(texX-stepSize,texY);
		 		glVertex3f(depth,x,y);
				glTexCoord2f(texX-stepSize,texY-stepSize);
				glVertex3f(depth,x,y+yStepSize);
				glTexCoord2f(texX,texY-stepSize);
				glVertex3f(depth,x+xStepSize,y+yStepSize);
				glTexCoord2f(texX,texY);
				glVertex3f(depth,x+xStepSize,y);
			}
		}
		
		glEnd();
	// In y plane
	} else if (plane == 1) {
		glBegin(GL_QUADS);
		for (GLfloat i = 0; i < 1; i += (1.0/resolution)) {
			for (GLfloat j = 0; j < 1; j += (1.0/resolution)) {
				GLfloat x = mapVal(i, 0, 1, corners[0][0], corners[1][0]);
				GLfloat y = mapVal(j, 0, 1, corners[0][1], corners[1][1]);
				GLfloat texX =i + stepSize;
				GLfloat texY = 1.0 - j;
				glTexCoord2f(texX-stepSize,texY);
		 		glVertex3f(x,depth,y);
				glTexCoord2f(texX-stepSize,texY-stepSize);
				glVertex3f(x,depth,y+yStepSize);
				glTexCoord2f(texX,texY-stepSize);
				glVertex3f(x+xStepSize,depth,y+yStepSize);
				glTexCoord2f(texX,texY);
				glVertex3f(x+xStepSize,depth,y);
			}
		}
		glEnd();
	// In z plane
	} else if (plane == 2) {
		glBegin(GL_QUADS);
		for (GLfloat i = 0; i < 1; i += (1.0/resolution)) {
			for (GLfloat j = 0; j < 1; j += (1.0/resolution)) {
				GLfloat x = mapVal(i, 0, 1, corners[0][0], corners[1][0]);
				GLfloat y = mapVal(j, 0, 1, corners[0][1], corners[1][1]);
				GLfloat texX =i + stepSize;
				GLfloat texY = 1.0 - j;
				glTexCoord2f(texX-stepSize,texY);
		 		glVertex3f(y,x,depth);
				glTexCoord2f(texX-stepSize,texY-stepSize);
				glVertex3f(y+yStepSize,x,depth);
				glTexCoord2f(texX,texY-stepSize);
				glVertex3f(y+yStepSize,x+xStepSize, depth);
				glTexCoord2f(texX,texY);
				glVertex3f(y, x+xStepSize,depth);
			}
		}
		glEnd();
	}
	glDisable(GL_TEXTURE_2D);
	
}

// Creates a room around 0, 0, 0 with dimentions 6x4x6
void CW2Widget::room() {

	// Normals for the walls
	GLfloat normals[][3] = {{-1., 0. ,0.}, {0., 0., 1.}, {0., 0., -1.}, {1., 0., 0.}, {0., 1., 0.}, {0., -1., 0.}, {0., 1., 0.}};

	glMaterialfv(GL_FRONT, GL_AMBIENT, mattWhiteMat.ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mattWhiteMat.diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, mattWhiteMat.specular);
	glMaterialf(GL_FRONT, GL_SHININESS, mattWhiteMat.shininess);

	glNormal3fv(normals[0]);
	GLfloat wall1[2][2] = {{0, -3.},{4,3.}};
	texturedPlane(wall1, 0, 3., 10, 2);

	glNormal3fv(normals[1]);
	GLfloat wall2[2][2] = {{0, -3.},{4,3.}};
	texturedPlane(wall2, 2, -3., 10, 2);

	glNormal3fv(normals[2]);

	GLfloat wall3[2][2] = {{0, -3.},{4,3.}};
	texturedPlane(wall3, 2, 3., 10, 2);

	// The wall on the x axis facing positive x will have the map on it
	glNormal3fv(normals[3]);

	GLfloat wall4[2][2] = {{0, -3.},{4,3.}};
	texturedPlane(wall4, 0, -3., 10,0);

	glNormal3fv(normals[4]);
	GLfloat floor[2][2] = {{-3,-3},{3,3}};
	texturedPlane(floor, 1, 0, 10,3);

	glNormal3fv(normals[5]);
	GLfloat ceiling[2][2] = {{-3,-3},{3,3}};
	texturedPlane(ceiling, 1, 4, 10,3);
}

// Creates a table at 0, 0, 0 that is 1 tall and 2x2
void CW2Widget::table() {
	glPushMatrix();

	glMaterialfv(GL_FRONT, GL_AMBIENT, mattBrownMat.ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mattBrownMat.diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, mattBrownMat.specular);
	glMaterialf(GL_FRONT, GL_SHININESS, mattBrownMat.shininess);

	// -0.7, 0, -0.7
	glTranslatef(-0.7,0.,-0.7);
	tableLeg();
	// 0.7, 0, -0.7
	glTranslatef(1.4,0.,0.);
	tableLeg();
	// 0.7, 0, 0.7
	glTranslatef(0.,0.,1.4);
	tableLeg();
	// -0.7, 0, 0.7
	glTranslatef(-1.4,0.,0.);
	tableLeg();
	// 0, 1, 0
	glTranslatef(.7,1.,-.7);
	tableTop();

	glPopMatrix();
}

void CW2Widget::tableLeg() {
	// Here are the normals, correctly calculated for the cube faces below
	GLfloat normals[][3] = {{-1., 0. ,0.}, {0., 0., 1.}, {0., 0., -1.}, {1., 0., 0.}};

	glNormal3fv(normals[0]);
	GLfloat side1[2][2] = {{0, -.05},{1,.05}};
	plane(side1, 0, -.05, 1);

	glNormal3fv(normals[1]);
	GLfloat side2[2][2] = {{0, -.05},{1,.05}};
	plane(side2, 2, .05, 1);

	glNormal3fv(normals[2]);

	GLfloat side3[2][2] = {{0, -.05},{1,.05}};
	plane(side3, 2, -.05, 1);

	glNormal3fv(normals[3]);

	GLfloat side4[2][2] = {{0, -.05},{1,.05}};
	plane(side4, 0, .05, 1);
	
}

void CW2Widget::tableTop() {
	// Here are the normals, correctly calculated for the cube faces below
	GLfloat normals[][3] = {{-1., 0. ,0.}, {0., 0., 1.}, {0., 0., -1.}, {1., 0., 0.}, {0.,1.,0.}};

	glNormal3fv(normals[0]);
	GLfloat side1[2][2] = {{0, -1},{0.1,1}};
	plane(side1, 0, -1, 1);

	glNormal3fv(normals[1]);
	GLfloat side2[2][2] = {{0, -1},{0.1,1}};
	plane(side2, 2, 1, 1);

	glNormal3fv(normals[2]);

	GLfloat side3[2][2] = {{0, -1},{0.1,1}};
	plane(side3, 2, -1, 1);

	glNormal3fv(normals[3]);

	GLfloat side4[2][2] = {{0, -1},{0.1,1}};
	plane(side4, 0, 1, 1);

	glNormal3fv(normals[4]);

	GLfloat top[2][2] = {{-1,-1},{1,1}};
	plane(top,1,0.1, 5);

}

// Places a robot at 0, 0, 0
void CW2Widget::robot() {
	glRotatef(robotAngle[0], 0, 1, 0);
	robotBase();
}

void CW2Widget::robotBase() {

	glMaterialfv(GL_FRONT, GL_AMBIENT, shinyRedMat.ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, shinyRedMat.diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, shinyRedMat.specular);
	glMaterialf(GL_FRONT, GL_SHININESS, shinyRedMat.shininess);

	glRotatef(-90,1,0,0);
	gluCylinder(roboBase, .3,.3, .2, 20, 5);
	glTranslatef(0, 0, .2);
	gluDisk(roboBaseTop, 0, .3, 20, 5);
	glRotatef(90,1,0,0);

	glTranslatef(.15,0,0);
	
	// Base - Arm connector
	GLfloat normals[][3] = {{-1., 0. ,0.}, {0., 0., 1.}, {0., 0., -1.}, {1., 0., 0.}, {0.,1.,0.}};
	
	glNormal3fv(normals[0]);
	GLfloat side1[2][2] = {{0, -.15},{0.3,.15}};
	plane(side1, 0, -0.05, 1);

	glNormal3fv(normals[1]);
	GLfloat side2[2][2] = {{0, -.05},{0.3,.05}};
	plane(side2, 2, 0.15, 1);

	glNormal3fv(normals[2]);

	GLfloat side3[2][2] = {{0, -.05},{0.3,.05}};
	plane(side3, 2, -0.15, 1);

	glMaterialfv(GL_FRONT, GL_AMBIENT, mattWhiteMat.ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mattWhiteMat.diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, mattWhiteMat.specular);
	glMaterialf(GL_FRONT, GL_SHININESS, mattWhiteMat.shininess);

	glNormal3fv(normals[3]);

	GLfloat side4[2][2] = {{0, -.15},{0.3,.15}};
	texturedPlane(side4, 0, 0.05, 1,1);

	glMaterialfv(GL_FRONT, GL_AMBIENT, shinyRedMat.ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, shinyRedMat.diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, shinyRedMat.specular);
	glMaterialf(GL_FRONT, GL_SHININESS, shinyRedMat.shininess);

	glNormal3fv(normals[4]);

	GLfloat top[2][2] = {{-.05,-.15},{.05,.15}};
	plane(top,1,0.3, 2);

	glTranslatef(-.15, .2, 0);
	robotArm();

}

void CW2Widget::robotArm() {

	glRotatef(-robotAngle[1], 1, 0, 0);
	glTranslatef(0,0,-.15);
	
	glRotatef(180, 1, 0, 0);
	gluDisk(roboArmBottom, 0, .1, 20, 5);
	glRotatef(-180, 1, 0, 0);
	
	gluCylinder(roboArm, .1,.1,2,20,5);
	
	glTranslatef(0, 0, 2);
	gluDisk(roboArmTop, 0, .1, 20, 5);

	glTranslatef(.14,0,-.1);
	robotTorch();
}

void CW2Widget::robotTorch() {

	glMaterialfv(GL_FRONT, GL_AMBIENT, mattBrownMat.ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mattBrownMat.diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, mattBrownMat.specular);
	glMaterialf(GL_FRONT, GL_SHININESS, mattBrownMat.shininess);

	glRotatef(robotAngle[1]-90, 1, 0, 0);
	glTranslatef(0,0,-0.15);

	glRotatef(180, 1, 0, 0);
	gluDisk(roboTorchHandleBottom, 0, .03, 10, 1);
	glRotatef(-180, 1, 0, 0);

	gluCylinder(roboTorchHandle, 0.03,0.08,0.45,10,3);

	glMaterialfv(GL_FRONT, GL_AMBIENT, flameMat.ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, flameMat.diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, flameMat.specular);
	glMaterialf(GL_FRONT, GL_SHININESS, flameMat.shininess);

	glTranslatef(0,0,.507);
	gluSphere(roboTorchFlame,0.113, 10,10);

	// Place torchLight
	GLfloat lightPos[] = {0,0,0};

	// Change the intesntiy modifier by a slight amount and cap between 0.7 and 1.3
	float deltaMod = ((float)std::rand()/(float)RAND_MAX - 0.5)/15.;
	torchModifier += deltaMod;
	if (torchModifier < .7) {
		torchModifier = .7;
	} else if (torchModifier > 1.3) {
		torchModifier = 1.3;
	}

	// Set the material for the light to be the torchLightMat, that has been modified randomly
	// This is to create the flickering torch effect.
	matStruct moddedLight = {
    { torchLightMat.ambient[0] * torchModifier, torchLightMat.ambient[1] * torchModifier, 
	torchLightMat.ambient[2] * torchModifier, torchLightMat.ambient[3] * torchModifier },
    { torchLightMat.diffuse[0] * torchModifier, torchLightMat.diffuse[1] * torchModifier, 
	torchLightMat.diffuse[2] * torchModifier, torchLightMat.diffuse[3] * torchModifier },
    { torchLightMat.specular[0] * torchModifier, torchLightMat.specular[1] * torchModifier, 
	torchLightMat.specular[2] * torchModifier, torchLightMat.specular[3] * torchModifier },
    0.
	};

	glLightfv(GL_LIGHT0,GL_POSITION, lightPos);
	glLightfv(GL_LIGHT0,GL_AMBIENT, moddedLight.ambient);
	glLightfv(GL_LIGHT0,GL_DIFFUSE, moddedLight.diffuse);
	glLightfv(GL_LIGHT0,GL_SPECULAR, moddedLight.specular);
	GLfloat lightAttenFactor = 2;
	glLightfv(GL_LIGHT0,GL_QUADRATIC_ATTENUATION,&lightAttenFactor);
}

// --------Utility Functions--------
// ---------------------------------

// Simple function for converting degrees to radians
GLfloat CW2Widget::degToRad(GLfloat deg) {
	return (deg * M_PI) / 180.;
}

void CW2Widget::hideMouse() {
		QCursor cursor;
		cursor.setPos(mapToGlobal(QPoint(width() / 2.,height()/2.)));
		cursor.setShape(Qt::BlankCursor);
		setCursor(cursor);
}

void CW2Widget::showMouse() {
	QCursor cursor;
	cursor.setShape(Qt::ArrowCursor);
	setCursor(cursor);
}

// Function to map a value from one range to another
// e.g. 2 mapped from [1,3] to [3,7] would be 5
float CW2Widget::mapVal(float value,float inStart, float inEnd, float outStart, float outEnd) {
	double ratio = (outEnd - outStart) / (inEnd - inStart);
	return outStart + ratio*(value - inStart);
}
