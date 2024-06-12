#include "disp_window.h"
#include <GLFW/glfw3.h>

#include <stdlib.h>
#include <cmath>


void disp_window::show() {
    /* Initialize the library */
    if (!glfwInit())
        LOAD_SUCCESS = 0;

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(750, 750, "Shape Display", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        LOAD_SUCCESS = 0;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);
    glClearColor(1., 1., 1., 1.);

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        /* Render here */
        glClear(GL_COLOR_BUFFER_BIT);

        drawMesh();

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();
    }

    glfwTerminate();
}


//void disp_window::mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
//
//}


void disp_window::loadMesh(fi2d_mesh inMesh) {
    this->mesh = inMesh;
}

fi2d_mesh disp_window::saveMesh() {
    return this->mesh;
}

void disp_window::drawMesh() {
    glColor4f(1., 0., 0., 1.);
    glLineWidth(2);
    glPushMatrix();

    // Scale so that the values at -1 and 1 are not touching the adges
    //glScalef(0.9, 0.9, 1);

    for (size_t i = 0; i < mesh.fCount; i++)
    {
        Face3i face = mesh.faces[i];
        Vert2f v1 = mesh.verts[face.v1];
        Vert2f v2 = mesh.verts[face.v2];
        Vert2f v3 = mesh.verts[face.v3];

        glBegin(GL_LINE_LOOP);
        glVertex2f(v1.x, v1.y);
        glVertex2f(v2.x, v2.y);
        glVertex2f(v3.x, v3.y);
        glEnd();
    }

    glPopMatrix();

}