#include "trigrid_window.h"
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <cmath>
#include <random>
#include <time.h>
#include <delaunator.hpp>

#define PI 3.14159265f
#define TESTA cout << "A" << endl;
#define TESTB cout << "B" << endl;

// This is defined so that the static callbacks know what instance of trigrid_window to pass the input to
trigrid_window* ACTIVEWIN;

// Constructor to initialise variables
trigrid_window::trigrid_window() {
    mDown = false;
    mXPos = 0.; mYPos = 0.;
    sVert = -1;
    scale = 0.6f;

    // Set the active window pointer to point to this instance of trigrid_window
    // This allows the instance to receive inputs
    ACTIVEWIN = this;
}

void trigrid_window::initGridVerts() {
    // Add a vert at each corner
    // This ensures all possible vert position will be inside the tri grid
    gridVerts.push_back(Vert2f{ -1,-1 });
    gridVerts.push_back(Vert2f{ 1,-1 });
    gridVerts.push_back(Vert2f{ -1, 1 });
    gridVerts.push_back(Vert2f{ 1, 1 });

    // Seed the randomiser
    srand((unsigned int)time(NULL));

    //Attempt to add some random vertices
    for (size_t i = 0; i < internalVCount; i++)
    {
        while (1) {
            // Generate a random number -1 to 1 with 2 decimal places
            int rX = rand() % 201 - 100;
            int rY = rand() % 201 - 100;
            float rXPos = (float)rX / 100.f;
            float rYPos = (float)rY / 100.f;

            // Check that it is not within 0.3 of any other point
            bool clear = true;
            for (size_t i = 0; i < gridVerts.size(); i++)
            {
                if ((getDistBetweenVerts(gridVerts[i], Vert2f{ rXPos, rYPos })) <= 0.3f) {
                    clear = false;
                }
            }
            // If it is not within 0.3f of any other point, add it to the trigrid
            if (clear) {
                gridVerts.push_back(Vert2f{ rXPos, rYPos });
                break;
            }
        }
    }

    gridVtoF.resize(gridVerts.size());
}

// Function to triangulate the verts in gridVerts using delauny triangulation
void trigrid_window::initGridFaces() {
    // Create a coordinate array that the delaunator can understand
    vector<double> coords;
    for (size_t i = 0; i < gridVerts.size(); i++) {
        coords.push_back((double)gridVerts[i].x);
        coords.push_back((double)gridVerts[i].y);
    }

    // Triangulate
    delaunator::Delaunator d(coords);

    // For each triangle made by the delaunator
    for (size_t i = 0; i < d.triangles.size(); i += 3) {
        // Get the index in gridVerts of the three corners of the face
        int vi1, vi2, vi3;
        vi1 = (int)d.triangles[ i ];
        vi2 = (int)d.triangles[i+1];
        vi3 = (int)d.triangles[i+2];

        // Add the face id to each of the vertices' gridVtoF element
        gridVtoF[vi1].push_back(i/3);
        gridVtoF[vi2].push_back(i/3);
        gridVtoF[vi3].push_back(i/3);

        // If the vertices are in counter-clockwise order
        if (isCCW(gridVerts[vi1], gridVerts[vi2], gridVerts[vi2])) {
            // Add the face to the gridFaces vector
            gridFaces.push_back(Face3i{ vi1,vi2,vi3 });
        }
        else {
            // Add the face to the gridFaces vector in reverse order
            gridFaces.push_back(Face3i{ vi3,vi2,vi1 });
        }

    }
}

// Function to set up the window and start the main loop
void trigrid_window::show() {
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

    /* Setup callbacks for input */
    glfwSetCursorPosCallback(window, this->cursor_position_callback);
    glfwSetMouseButtonCallback(window, this->mouse_button_callback);

    // Initialise the triangle grid
    initGridVerts();
    initGridFaces();
    fillGridFaces();

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    // Set bg color
    glClearColor(1., 1., 1., 1.);

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        /* Render here */
        glClear(GL_COLOR_BUFFER_BIT);

        /* Main loop drawing */
        drawMesh();
        drawGrid();

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();
    }

    glfwTerminate();
}

// Called by the mouse button callback when button is pressed or released
void trigrid_window::mbUpdate(int button, int action) {
    // If the callback was triggered by a press
    if (action == GLFW_PRESS and button == GLFW_MOUSE_BUTTON_1) {
        mDown = true;

        // Get the x and y of the mouse in world space (-1 to 1)
        float xPos, yPos;
        getMousePos(&xPos, &yPos);

        // Find if the mouse is close to any point
        for (size_t i = 0; i < gridVerts.size(); i++) {
            float d = getDistBetweenVerts(gridVerts[i], Vert2f{ xPos, yPos });
            // If sufficiently close, set that vertex as being edited
            if (d <= 0.02) {
                sVert = i;
                return;
            }
        }
    }
    // If the callback was triggered by a release
    else if (action == GLFW_RELEASE and button == GLFW_MOUSE_BUTTON_1) {
        mDown = false;
        sVert = -1;
    }
}

// Triggered by the mouse movement callback
void trigrid_window::mpUpdate(double xPos, double yPos) {
    // Set the mouse position to the value passed by the callback
    mXPos = xPos;
    mYPos = yPos;

    // If the left mouse button is currently pressed
    if (mDown and sVert != -1) {
        // Get the world space mouse coordinates
        float rXPos, rYPos;
        getMousePos(&rXPos, &rYPos);

        changeGrid(rXPos, rYPos);
    }
}

// This function fills the gridSecs array
// For each grid section it stores the vertices inside and their bilinear interpolation ratios
void trigrid_window::fillGridFaces() {
    // Create an array, one element per vertex, storing if that element has been added to the gridSecs array
    vector<bool> vertsAdded;
    for (size_t i = 0; i < mesh.vCount; i++) {
        vertsAdded.push_back(false);
    }

    // for each grid section
    for (size_t i = 0; i < gridFaces.size(); i++)
    {
        Face3i currFace = gridFaces[i];
        Vert2f currFV1 = gridVerts[currFace.v1];
        Vert2f currFV2 = gridVerts[currFace.v2];
        Vert2f currFV3 = gridVerts[currFace.v3];

        vector<tuple<int, float, float>> currFaceContents;

        // For each vert
        for (size_t v = 0; v < mesh.vCount; v++) {
            // If it hasn't been added already
            if (vertsAdded[v])
                continue;

            //cout << "Face " << i << " checking vert " << v << endl;
            // Check if it is in the quad section
            if (vertInside(mesh.verts[v], currFV1, currFV2, currFV3)) {
                vertsAdded[v] = true;

                float alpha, beta;
                invBaryInterp(mesh.verts[v], currFV1, currFV2, currFV3, &alpha, &beta);

                // Add the vertex and the ratios to this grid sections vector
                currFaceContents.push_back(make_tuple(v, alpha, beta));
            }
        }
        gridSecs.push_back(currFaceContents);
        //cout << "Face " << i << ": " << gridSecs[i].size() << " verts" << endl;
    }

}

// Calculate the barycentric coordinates of the point p in the triangle abc
// Note: Only alpha and bet aare calculated as gamma is implied as 1-alpha-beta
void trigrid_window::invBaryInterp(Vert2f p, Vert2f a, Vert2f b, Vert2f c, float* alpha, float* beta) {

    // This is the equasion to calculate alpha and beta in it's simplest form
    float bot = ((b.y - c.y) * (a.x - c.x)) + ((c.x - b.x) * (a.y - c.y));
    float aTop = ((b.y - c.y) * (p.x - c.x)) + ((c.x - b.x) * (p.y - c.y));
    float bTop = ((c.y - a.y) * (p.x - c.x)) + ((a.x - c.x) * (p.y - c.y));

    // Alpha and beta are returned
    *alpha = aTop / bot;
    *beta = bTop / bot;
}

// Use the barycentric coords to find the cartesian coords of point p in the triangle abc, with barycentric coords: (alpha, beta, 1- alpha - beta)
void trigrid_window::baryInterp(Vert2f* p, Vert2f a, Vert2f b, Vert2f c, float alpha, float beta) {
    float xOut = (a.x * alpha) + (b.x * beta) + (c.x * (1 - alpha - beta));
    float yOut = (a.y * alpha) + (b.y * beta) + (c.y * (1 - alpha - beta));
    *p = Vert2f{ xOut,yOut };
}

// Called while setting up the object, loads a mesh into the instance
void trigrid_window::loadMesh(fi2d_mesh inMesh) {
    this->mesh = inMesh;
}

// Returns the edited mesh
fi2d_mesh trigrid_window::saveMesh() {
    return this->mesh;
}

// Uses old school opengl to draw the mesh to screen
void trigrid_window::drawMesh() {
    glColor4f(1., 0., 0., 1.);
    glLineWidth(2);
    glPushMatrix();

    // Scale so that the values at -1 and 1 are not touching the adges
    glScalef(scale, scale, 1.f);

    // For each face in the mesh, draw it as a line loop (outline of the face)
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

// Draw the grid over the mesh on screen
void trigrid_window::drawGrid() {
    glColor4f(0., 0., 0., 1.);
    glPushMatrix();

    // Scale so that the values at -1 and 1 are not touching the adges
    glScalef(scale, scale, 1);

    // For each face in the mesh, draw it as a line loop (outline of the face)
    for (size_t i = 0; i < gridFaces.size(); i++)
    {
        Face3i face = gridFaces[i];
        Vert2f v1 = gridVerts[face.v1];
        Vert2f v2 = gridVerts[face.v2];
        Vert2f v3 = gridVerts[face.v3];

        //glBegin(GL_TRIANGLES);
        glBegin(GL_LINE_LOOP);
        glVertex2f(v1.x, v1.y);
        glVertex2f(v2.x, v2.y);
        glVertex2f(v3.x, v3.y);
        glEnd();
    }

    glPointSize(3);
    glBegin(GL_POINTS);
    for (size_t i = 0; i < gridVerts.size(); i++)
    {
        glVertex2f(gridVerts[i].x, gridVerts[i].y);
    }
    glEnd();

    glPopMatrix();
}

// Move the selected grid vertex in sVert to the coordinates (x,y)
void trigrid_window::changeGrid(float x, float y) {
    // Move the grid vertex
    gridVerts[sVert] = Vert2f{ x,y };

    for (size_t i = 0; i < gridVtoF[sVert].size(); i++) {
        changeGridSec(gridVtoF[sVert][i]);
    }
}

// Update all the vertices of the face 
void trigrid_window::changeGridSec(size_t fIndex) {
    //// For each vertex in the grid section
    for (size_t i = 0; i < gridSecs[fIndex].size(); i++) {
        Face3i currFace = gridFaces[fIndex];
        Vert2f currFV1 = gridVerts[currFace.v1];
        Vert2f currFV2 = gridVerts[currFace.v2];
        Vert2f currFV3 = gridVerts[currFace.v3];
        tuple<int, float, float> vTuple = gridSecs[fIndex][i];
        // Do bilinear interpolation to find the new x,y coords of the vertex
        // output the new x and y to newX and newY
        Vert2f newPoint;
        baryInterp(&newPoint, currFV1, currFV2, currFV3, get<1>(vTuple), get<2>(vTuple));

        int vIndex = get<0>(vTuple);

        mesh.verts[vIndex] = newPoint;
    }

}

// Find if a vert at point p, is in the triangle made of points c1-c3
// The values of c1-c3 should represent the corners of a triangle in the order:
//     c3
//    /|
//   / |
// c1--c2
bool trigrid_window::vertInside(Vert2f p, Vert2f c1, Vert2f c2, Vert2f c3) {
    bool aboveA = false;
    bool aboveB = false;
    bool aboveC = false;
    // Check for each of the four sides of the quad, that the point p is "above" the line
    aboveA = (halfPlane(p, c1, c2) >= 0);
    aboveB = (halfPlane(p, c2, c3) >= 0);
    aboveC = (halfPlane(p, c3, c1) >= 0);

    // p is inside the triangle if it is above all 3 sides
    return (aboveA and aboveB and aboveC);
}

// Use the half plane test to determine the distance from the line ab that p is
// Positive = above the line, negative = below, 0 = on the line
float trigrid_window::halfPlane(Vert2f p, Vert2f a, Vert2f b) {
    // Find the normal and the magnituse of the normal of line ab
    Vert2f n{ -(b.y - a.y), b.x - a.x };
    float absN = sqrt(pow(n.x, 2) + pow(n.y, 2));

    // find the line ap
    Vert2f ap{ p.x - a.x , p.y - a.y };

    // Find the dot produce: n.(ap)
    float dProd = (n.x * ap.x) + (n.y * ap.y);

    // Return the distance from the line that p lies
    return dProd / absN;
}

// Returns the coordinates of the mouse in world coordinates (-1 to 1) in both x and y
void trigrid_window::getMousePos(float* x, float* y) {

    // Get window dimentions
    int w, h;
    glfwGetWindowSize(window, &w, &h);

    // Calculate world coordinates
    *x = ((((float)mXPos / ((float)w / 2.f))) - 1) / scale;
    *y = (((((float)mYPos / ((float)h / 2.f))) - 1) * -1) / scale;

}

// Find the distance between two vertices using pythagoras
float trigrid_window::getDistBetweenVerts(Vert2f a, Vert2f b) {
    float dX = a.x - b.x;
    float dY = a.y - b.y;
    float sqrSum = pow(dX, 2) + pow(dY, 2);
    float dist = sqrt(sqrSum);
    return dist;
}

// Function that takes in the three corners of a triangle
// Returns a bool: whether the three verts are given in counter-clockwise order
float trigrid_window::isCCW(Vert2f a, Vert2f b, Vert2f c) {
    // Find the midpoint of the triangle
    Vert2f m{ (a.x + b.x + c.x) / 3.f,(a.y + b.y + c.y) / 3.f };

    // Find the vectors from the mid points to the corners
    Vert2f ma{ a.x - m.x,a.y - m.y };
    Vert2f mb{ b.x - m.x,b.y - m.y };
    Vert2f mc{ c.x - m.x,c.y - m.y };

    // Calculate the angle CCW (0-360) of each of the vectors from the vector (1,0)
    float alphaA = atan2(ma.y, ma.x) * 180.f / PI;
    if (alphaA < 0)
        alphaA += 360.f;
    float alphaB = atan2(mb.y, mb.x) * 180.f / PI;
    if (alphaB < 0)
        alphaB += 360.f;
    float alphaC = atan2(mc.y, mc.x) * 180.f / PI;
    if (alphaC < 0)
        alphaC += 360.f;

    // Return whether or not the verts are in CCW order
    if (alphaC > alphaA) {
        return (alphaB > alphaA) and (alphaB < alphaC);
    }
    else {
        return (alphaB < alphaC) or (alphaB > alphaA);
    }
}

// Static callback member declarations
void trigrid_window::mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    ACTIVEWIN->mbUpdate(button, action);
}

void trigrid_window::cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    ACTIVEWIN->mpUpdate(xpos, ypos);
}