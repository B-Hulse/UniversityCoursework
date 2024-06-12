#pragma once
#include <GLFW/glfw3.h>
#include "fi2d_mesh.h"

class disp_window
{
private:
    GLFWwindow* window = NULL;
    fi2d_mesh mesh;

    void drawMesh();
public:
    int LOAD_SUCCESS = 1;

    void show();

    void loadMesh(fi2d_mesh mesh);
    fi2d_mesh saveMesh();
};