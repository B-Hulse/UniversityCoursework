#include "grid_window.h"
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <cmath>

// This is defined so that the static callbacks know what instance of grid_window to pass the input to
grid_window* ACTIVEGRIDWIN;

// Constructor to initialise variables
grid_window::grid_window() {
    mDown = false;
    mXPos = 0.; mYPos = 0.;
    sVert = Vert2f{ 0,0 };

    // Place each grid vertex evenly distributed between -1 and 1 in both x and y
    for (int x = 0; x < 11; x++) {
        for (int y = 0; y < 11; y++) {
            grid[x][y] = Vert2f{ ((float)x / 5.f) - 1.f,((float)y / 5.f) - 1.f };
        }
    }

    // Set the active window pointer to point to this instance of grid_window
    // This allows the instance to receive inputs
    ACTIVEGRIDWIN = this;
}

// Function to set up the window and start the main loop
void grid_window::show() {
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
void grid_window::mbUpdate(int button, int action) {
    // If the callback was triggered by a press
    if (action == GLFW_PRESS and button == GLFW_MOUSE_BUTTON_1) {
        mDown = true;

        // Get the x and y of the mouse in world space (-1 to 1)
        float xPos, yPos;
        getMousePos(&xPos, &yPos);
        
        // Find if the mouse is close to any point
        for (size_t i = 0; i < 11; i++) {
            for (size_t j = 0; j < 11; j++) {
                float d = getDistBetweenVerts(grid[i][j], Vert2f{ xPos, yPos });
                // If sufficiently close, set that vertex as being edited
                if (d <= 0.02) {
                    sVert.x = (float)i;
                    sVert.y = (float)j;
                    return;
                }
            }
        }
    }
    // If the callback was triggered by a release
    else if (action == GLFW_RELEASE and button == GLFW_MOUSE_BUTTON_1) {
        mDown = false;
    }
}

// Triggered by the mouse movement callback
void grid_window::mpUpdate(double xPos, double yPos) {
    // Set the mouse position to the value passed by the callback
    mXPos = xPos;
    mYPos = yPos;

    // If the left mouse button is currently pressed
    if (mDown) {
        // Get the world space mouse coordinates
        float rXPos, rYPos;
        getMousePos(&rXPos, &rYPos);

        changeGrid(rXPos, rYPos);
    }
}


// This function fills the gridSecs array
// For each grid section it stores the vertices inside and their bilinear interpolation ratios
void grid_window::interpVerts() {
    // Create an array, one element per vertex, storing if that element has been added to the gridSecs array
    vector<bool> vertsAdded;
    for (size_t i = 0; i < mesh.vCount; i++) {
        vertsAdded.push_back(false);
    }

    // for each grid section
    for (size_t i = 0; i < 100; i++)
    {
        // Calculate the coordinates in the grid array of the top left of this grid section
        size_t secX = i%10;
        size_t secY = (size_t)floor((float)i/10.f);

        // For each vert
        for (size_t v = 0; v < mesh.vCount; v++) {
            // If it hasn't been added already
            if (vertsAdded[v])
                continue;

            // Check if it is in the quad section
            if (vertInside(mesh.verts[v], grid[secX][secY], grid[secX+1][secY], grid[secX + 1][secY + 1], grid[secX][secY+1])) {
                vertsAdded[v] = true;

                // The three ratios needed for bilinear interp
                // Values 0 <= r <= 1
                float rTop, rBot, rMid;

                // Calculate the bilinear interpolation ratios
                // This can be done with such ease as we know every grid section starts as a 0.2x0.2 square
                rTop = rBot = (mesh.verts[v].x - grid[secX][secY].x) / 0.2f;
                rMid = (mesh.verts[v].y - grid[secX][secY].y) / 0.2f;

                // Add the vertex and the ratios to this grid sections vector
                gridSecs[i].push_back(make_tuple(v,rTop,rBot,rMid));
            }
        }
    }

}

// Called while setting up the object, loads a mesh into the instance
void grid_window::loadMesh(fi2d_mesh inMesh) {
    this->mesh = inMesh;

    interpVerts();
}

// Returns the edited mesh
fi2d_mesh grid_window::saveMesh() {
    return this->mesh;
}

// Uses old school opengl to draw the mesh to screen
void grid_window::drawMesh() {
    glColor4f(1., 0., 0., 1.);
    glLineWidth(2);
    glPushMatrix();

    // Scale so that the values at -1 and 1 are not touching the adges
    glScalef(0.9f, 0.9f, 1.f);

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
void grid_window::drawGrid() {
    glColor4f(0., 0., 0., 1.);
    glPushMatrix();

    // Scale so that the values at -1 and 1 are not touching the adges
    glScalef(0.9f, 0.9f, 1);

    // For each row and column in the grid, draw a line connecting the vertex
    glLineWidth(1);
    for (size_t col = 0; col < 11; col++) {
        glBegin(GL_LINE_STRIP);
        for (size_t row = 0; row < 11; row++) {
            glVertex2f(grid[col][row].x, grid[col][row].y);
        }
        glEnd();
    }
    for (size_t row = 0; row < 11; row++) {
        for (size_t col = 0; col < 11; col++) {
            glBegin(GL_LINE_STRIP);
            glVertex2f(grid[col][row].x, grid[col][row].y);
        }
        glEnd();
    }

    // Draw a point at each vertex of the grid
    glPointSize(3);
    glBegin(GL_POINTS);
    for (size_t i = 0; i < 11; i++) {
        for (size_t j = 0; j < 11; j++) {
            glColor4f(0., 0., 0., 1.);
            glVertex2f(grid[i][j].x, grid[i][j].y);
        }
    }
    glEnd();

    glPopMatrix();
}

// Move the selected grid vertex in sVert to the coordinates (x,y)
void grid_window::changeGrid(float x, float y) {
    // Move the grid vertex
    grid[(int)sVert.x][(int)sVert.y].x = x;
    grid[(int)sVert.x][(int)sVert.y].y = y;

    // If the selected vertex is not the top right vertex of the grid
    if (sVert.x < 10 and sVert.y < 10) {
        // Update the gridSection NE of the selected vertex
        changeGridSec((size_t)sVert.x, (size_t)sVert.y);
    }
    // If the selected vertex is not the bottom right vertex of the grid
    if (sVert.x < 10 and sVert.y > 0) {
        // Update the gridSection SE of the selected vertex
        changeGridSec((size_t)sVert.x, (size_t)sVert.y-1);
    }
    // If the selected vertex is not the top left vertex of the grid
    if (sVert.x > 0 and sVert.y < 10) {
        // Update the gridSection NW of the selected vertex
        changeGridSec((size_t)sVert.x-1, (size_t)sVert.y);
    }
    // If the selected vertex is not the top left vertex of the grid
    if (sVert.x > 0 and sVert.y > 0) {
        // Update the gridSection SW of the selected vertex
        changeGridSec((size_t)sVert.x-1, (size_t)sVert.y-1);
    }


}

// Update all the vertices in the grid section with the bottom right corner vertex indices gsX, gsY
void grid_window::changeGridSec(size_t gsX, size_t gsY) {
    // Calculate the index of the grid section being updated
    size_t gsIndex = (size_t)(gsX + (gsY * 10));

    // For each vertex in the grid section
    for (size_t i = 0; i < gridSecs[gsIndex].size(); i++) {
        tuple<int, float, float, float> vTuple = gridSecs[gsIndex][i];

        // Do bilinear interpolation to find the new x,y coords of the vertex
        // output the new x and y to newX and newY
        float newX, newY;
        blerp(grid[(int)gsX][(int)gsY],
            grid[(int)gsX + 1][(int)gsY],
            grid[(int)gsX + 1][(int)gsY + 1],
            grid[(int)gsX][(int)gsY + 1],
            get<1>(vTuple), get<2>(vTuple), get<3>(vTuple),
            &newX, &newY);

        // Update the position of the vertex being moved
        mesh.verts[get<0>(vTuple)].x = newX;
        mesh.verts[get<0>(vTuple)].y = newY;
    }
}

// Bilinear interpolation using the ratios given into the quad with corners at c1-c4
// Result is given as coordinates in the values of xOut and yOut
// The values of c1-c4 should represent the corners of a quad in the order:
// c4---c3
// |    |
// |    |
// c1---c2
void grid_window::blerp(Vert2f c1, Vert2f c2, Vert2f c3, Vert2f c4, float rTop, float rBot, float rMid, float* xOut, float* yOut) {
    Vert2f top{ ((1-rTop) * c1.x) + (rTop * c2.x), ((1 - rTop) * c1.y) + (rTop * c2.y) };
    Vert2f bot{ ((1 - rBot) * c4.x) + (rBot * c3.x), ((1 - rBot) * c4.y) + (rBot * c3.y) };
    Vert2f mid{ ((1 - rMid) * top.x) + (rMid * bot.x), ((1 - rMid) * top.y) + (rMid * bot.y) };
    
    *xOut = mid.x;
    *yOut = mid.y;
}

// Find if a vert at point p, is in the quad made of points c1-c4
// The values of c1-c4 should represent the corners of a quad in the order:
// c4---c3
// |    |
// |    |
// c1---c2
bool grid_window::vertInside(Vert2f p, Vert2f c1, Vert2f c2, Vert2f c3, Vert2f c4) {
    bool aboveA = false;
    bool aboveB = false;
    bool aboveC = false;
    bool aboveD = false;
    // Check for each of the four sides of the quad, that the point p is "above" the line
    aboveA = (halfPlane(p, c1, c2) >= 0);
    aboveB = (halfPlane(p, c2, c3) >= 0);
    aboveC = (halfPlane(p, c3, c4) >= 0);
    aboveD = (halfPlane(p, c4, c1) >= 0);

    // p is inside the quad if it is above all 4 sides
    return (aboveA and aboveB and aboveC and aboveD);
}

// Use the half plane test to determine the distance from the line ab that p is
// Positive = above the line, negative = below, 0 = on the line
float grid_window::halfPlane(Vert2f p, Vert2f a, Vert2f b) {
    // Find the normal and the magnituse of the normal of line ab
    Vert2f n{ -(b.y - a.y), b.x - a.x };
    float absN = sqrt(pow(n.x,2)+pow(n.y,2));

    // find the line ap
    Vert2f ap{ p.x - a.x , p.y - a.y };

    // Find the dot produce: n.(ap)
    float dProd = (n.x * ap.x) + (n.y * ap.y);

    // Return the distance from the line that p lies
    return dProd / absN;
}

// Returns the coordinates of the mouse in world coordinates (-1 to 1) in both x and y
void grid_window::getMousePos(float* x, float* y) {

    // Get window dimentions
    int w, h;
    glfwGetWindowSize(window, &w, &h);

    // Calculate world coordinates
    *x = ((((float)mXPos / ((float)w / 2.f))) - 1) / 0.9f;
    *y = (((((float)mYPos / ((float)h / 2.f))) - 1) * -1) / 0.9f;

}

// Find the distance between two vertices using pythagoras
float grid_window::getDistBetweenVerts(Vert2f a, Vert2f b) {
    float dX = a.x - b.x;
    float dY = a.y - b.y;
    float sqrSum = pow(dX, 2) + pow(dY, 2);
    float dist = sqrt(sqrSum);
    return dist;
}

// Static callback member declarations
void grid_window::mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    ACTIVEGRIDWIN->mbUpdate(button, action);
}

void grid_window::cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    ACTIVEGRIDWIN->mpUpdate(xpos, ypos);
}