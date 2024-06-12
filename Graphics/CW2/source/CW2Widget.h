#ifndef __CW2_WIDGET_H__
#define __CW2_WIDGET_H__ 1

#include <map>
#include <vector>
#include <QGLWidget>
#include <GL/glut.h>
#include "CW2PPMImg.h"
#include "CW2MatWindow.h"
#include "CW2Materials.h"


class CW2Widget: public QGLWidget {
		Q_OBJECT
	public:
        CW2Widget(QWidget *parent);
        ~CW2Widget();

	public slots:
        void updateGL();
        void updateLight(GLfloat vals[3][4], GLfloat shin);

        // Comments explaining functions are found in the cpp file
	protected:
        void initializeGL();
        void loadTextures();
        void resizeGL(int w, int h);
        void paintGL();
        void room();
        void table();
        void tableLeg();
        void tableTop();
        void robot();
        void robotBase();
        void robotArm();
        void robotTorch();
        void plane(GLfloat corners[2][2], int plane, GLfloat depth, GLfloat resolution);
        void texturedPlane(GLfloat corners[2][2], int plane, GLfloat depth, GLfloat resolution, int tex_i);
        void mouseMoveEvent(QMouseEvent *event);
        void keyPressEvent(QKeyEvent *event);
        void keyReleaseEvent(QKeyEvent *event);
        void mousePressEvent(QMouseEvent *event);
        void mouseReleaseEvent(QMouseEvent *event);
        void handleKeys();
        void updateCamera();
        void showMenu();
        GLfloat degToRad(GLfloat deg);
        void hideMouse();
        void showMouse();
        float mapVal(float value,float inStart, float inEnd, float outStart, float outEnd);

        // Comments explainign attributes can be found in cpp file with initializations
	private:
        GLfloat cameraPos[3];
        GLfloat cameraDir[2];
        GLfloat cameraVel[3];
        std::map<int, bool> keysPressed;
        bool mousePressed;
        int frameRate;
        GLfloat robotAngle[2];
        GLUquadricObj* roboBase;
        GLUquadricObj* roboBaseTop;
        GLUquadricObj* roboArm;
        GLUquadricObj* roboArmTop;
        GLUquadricObj* roboArmBottom;
        GLUquadricObj* roboTorchHandle;
        GLUquadricObj* roboTorchHandleBottom;
        GLUquadricObj* roboTorchFlame;
        GLuint textures[4];
        matStruct torchLightMat;
        GLfloat torchModifier;
        CW2MatWindow* matWindow;
        bool robotAuto;

        // Some Default materials from http://www.it.hiof.no/~borres/j3d/explain/light/p-materials.html

        // Matt white plastic
        const matStruct mattWhiteMat = {
                {.05,.05,.05,1.0},
                {.5,.5,.5,1.0},
                {.5,.5,.5,1.0},
                0.1
        };

        // Siny red metal
        const matStruct shinyRedMat = {
                { 0.1745, 0.01175, 0.01175, 0.55 },
                {0.61424, 0.04136, 0.04136, 0.55 },
                {0.727811, 0.626959, 0.626959, 0.55 },
                76.8
        };
        // Matt Brown Material
        const matStruct mattBrownMat = {
                {0.25, 0.148, 0.06475, 1.0  },
                {0.4, 0.2368, 0.1036, 1.0 },
                {0.0774597, 0.0458561, 0.0200621, 1.0 },
                0.1
        };
        // White Light Material
        const matStruct flameMat = {
                { 1., .5, 0., 1. },
                { 1., .5, 0., 1. },
                { 1., .5, 0., 1. },
                1.
        };
};
	
#endif
