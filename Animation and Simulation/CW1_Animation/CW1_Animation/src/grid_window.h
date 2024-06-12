#pragma once
#include <GLFW/glfw3.h>
#include "fi2d_mesh.h"
#include <tuple>

class grid_window
{
private:
    GLFWwindow* window = NULL;
    fi2d_mesh mesh;

    // Variables to store the grid used to deform the mesh
    Vert2f grid[11][11];
    vector<tuple<int, float, float, float>> gridSecs[100];
    Vert2f sVert; // The vertex of the grid being moved

    bool mDown;
    double mXPos, mYPos;

    void interpVerts();

    void drawMesh();
    void drawGrid();

    // Input handling fucntions
    void getMousePos(float* x, float* y);
    void mbUpdate(int button, int action);
    void mpUpdate(double xPos, double yPos);

    // Functions for updating the vertex positions of the mesh once the grid has been edited
    void changeGrid(float x, float y);
    void changeGridSec(size_t gsX, size_t gsY);

    // Utility functions
    bool vertInside(Vert2f point, Vert2f c1, Vert2f c2, Vert2f c3, Vert2f c4);
    float halfPlane(Vert2f point, Vert2f a, Vert2f b);
    void blerp(Vert2f c1, Vert2f c2, Vert2f c3, Vert2f c4, float rTop, float rBot, float rMid, float *xOut, float *yOut);
    static float getDistBetweenVerts(Vert2f a, Vert2f b);
public:
    int LOAD_SUCCESS = 1;

    grid_window();
    
    void show();

    void loadMesh(fi2d_mesh mesh);
    fi2d_mesh saveMesh();


    // Static Callbacks
    static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
    static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
};