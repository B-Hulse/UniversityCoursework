#include "vendors/imgui/imgui.h"
#include "vendors/imgui/imgui_impl_opengl3.h"
#include "vendors/imgui/imgui_impl_glfw.h"

//#include "GL/glew.h"
//#include "GLFW/glfw3.h"

#include <glm/matrix.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>

#include "FluidSimulator.h"
#include "vendors/lodepng/lodepng.h"

#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <stdint.h>

using namespace std;

FluidSimulator::FluidSimulator() {
    if (!init()) {}
}

void FluidSimulator::cleanup() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (fluid) delete fluid;

    glfwDestroyWindow(window);
    glfwTerminate();
}

// initialise GLFW, glew and ImGui
int FluidSimulator::init() {
    // init GLFW
    if (!glfwInit()) {
        cout << "ERROR: GLFW init failed" << endl;
        return 0;
    }

    glewExperimental = GL_TRUE;

    // Define GLSL version
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(WINWIDTH, WINHEIGHT, "Shape Display", NULL, NULL);
    if (!window)
    {
        cout << "Error initialising window" << endl;
        glfwTerminate();
        return 0;
    }

    // Buffer size
    glfwGetFramebufferSize(window, &BUFWIDTH, &BUFHEIGHT);

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    // Initialise Glew
    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
        /* Problem: glewInit failed, something is seriously wrong. */
        fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
        return 0;
    }

    glViewport(0, 0, BUFWIDTH, BUFHEIGHT);

    // Set camera properties
    glClearColor(1.f, 1.f, 1.f, 1.00f);

    // Initialise ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    // initialise ImGui inputs
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    frameNo = 0;
    currFrameTime = 0.;
    prevFrameTime = 0.;

    desiredFrameTime = 0.01f;

    fluid = new Fluid(resetSize[0],resetSize[1]);

    return 1;
}

void FluidSimulator::run() {
    mainLoop();
    cleanup();
}

void FluidSimulator::mainLoop() {
    prevFrameTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {

        // Handle input
        glfwPollEvents();

        // ------- OpenGL -------

        // Clear Screen
        glClear(GL_COLOR_BUFFER_BIT);

        // Update Fluid
        if (playing) {
            if (fluid) {
                fluid->update(pressureKernel,viscosity);
            }
            frameNo++;
        }

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glPushMatrix();

        // Canvas is Bottom Left (0,0) to  Top Right (10,10)
        glScalef(.2f, .2f, 1.f);
        glTranslatef(-5.f, -5.f, 0.f);

        // Render stuff here

        // Render Floor
        glColor3f(0.5, 0.5, 0.5);
        glBegin(GL_QUADS);
        glVertex2f(0., 1.);
        glVertex2f(0., 0.);
        glVertex2f(10., 0.);
        glVertex2f(10., 1.);
        glEnd();

        // Render Tank
        //glBegin(GL_LINE_STRIP);
        //glVertex2f(2.5, 6.);
        //glVertex2f(2.5, 1.);
        //glVertex2f(7.5, 1.);
        //glVertex2f(7.5, 6.);
        //glEnd();
        glColor3f(0., 0., 0.);
        glBegin(GL_QUADS);
        // LWall
        glVertex2f(2.5, 6);
        glVertex2f(2.4, 6);
        glVertex2f(2.4, 1);
        glVertex2f(2.5, 1);
        // RWall
        glVertex2f(7.5, 6);
        glVertex2f(7.59, 6);
        glVertex2f(7.59, 1);
        glVertex2f(7.5, 1);
        // Base
        glVertex2f(2.5, 1);
        glVertex2f(7.5, 1);
        glVertex2f(7.5, 1.1);
        glVertex2f(2.5, 1.1);
        glEnd();

        // Render Fluid
        if (fluid) {
            fluid->render();
        }

        glPopMatrix();

        if (recording and playing) {
            savePNG();
        }

        if (frameStep) {
            frameStep = false;
            playing = false;
        }

        // ------- ImGui -------

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        imGuiDisplay();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);

        //// Get new frame timings
        //prevFrameTime = currFrameTime;
        //currFrameTime = glfwGetTime();
        //deltaFrameTime = currFrameTime - prevFrameTime;

        // Calculate frame timings and sleep to hit desired framerate
        float frameTimeRemaining = (float)(desiredFrameTime - (glfwGetTime() - currFrameTime));
        if (frameTimeRemaining > 0) {
            this_thread::sleep_for(chrono::milliseconds((int)(frameTimeRemaining * 1000)));
        }
        prevFrameTime = currFrameTime;
        currFrameTime = glfwGetTime();
        deltaFrameTime = currFrameTime - prevFrameTime;
    }
}

void FluidSimulator::savePNG() {
    //if (frameNo % 4 != 0) return;
    string filepath = "pngs/outpng" + to_string(frameNo/4) + ".png";
    int pCount = WINWIDTH * WINHEIGHT * 4;
    // get the pixel data from the frame buffer
    unsigned char* pixels = new unsigned char[pCount];
    glReadPixels(0, 0, WINWIDTH, WINHEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    vector<uint8_t> pix(pixels, pixels + pCount);
    delete[] pixels;

    unsigned error = lodepng::encode(filepath.c_str(), pix, WINWIDTH, WINHEIGHT);

    if (error) cout << "Error saving png" << error << ": " << lodepng_error_text(error) << endl;
}

void FluidSimulator::keyboardCheck() {
    // Camera Position Movement
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
    }
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
    }

    // Camera Direction Movement
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
    }
}

void FluidSimulator::mouseCallback(int button, int action, int mods) {
    // Left mouse click
    if (button == GLFW_MOUSE_BUTTON_1 and action == GLFW_PRESS) {
    }
}

void FluidSimulator::showMenuBar() {
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Exit")) {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::Checkbox("Info GUI", &showInfo);
            ImGui::Checkbox("Controls GUI", &showControls);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void FluidSimulator::showInfoGUI() {
    ImGui::Begin("Info");
    int framerate = (int)floor(1.0 / (double)deltaFrameTime);
    ImGui::Text("Framerate: %i", framerate);
    ImGui::Text("Frame Time: %f", deltaFrameTime);
    ImGui::Text("Frame: %i", frameNo);
    ImGui::End();
}

void FluidSimulator::showControlsGUI() {
    ImGui::Begin("Controls");
    if (ImGui::Button(playing ? "Pause" : "Play")) {
        playing = !playing;
    }
    ImGui::SameLine();
    ImGui::Checkbox("Record", &recording);
    if (ImGui::Button("Frame Step")) {
        frameStep = true;
        playing = true;
    }
    if (ImGui::BeginCombo("Pressure Kernel ", kernelStr[pressureKernel])) {
        // poly6
        if (ImGui::Selectable(kernelStr[Fluid::Kernel::poly6], pressureKernel == Fluid::Kernel::poly6)) {
            pressureKernel = Fluid::Kernel::poly6;
        }
        if (pressureKernel == Fluid::Kernel::poly6) ImGui::SetItemDefaultFocus();

        // Debrun
        if (ImGui::Selectable(kernelStr[Fluid::Kernel::Debrun], pressureKernel == Fluid::Kernel::Debrun)) {
            pressureKernel = Fluid::Kernel::Debrun;
        }
        if (pressureKernel == Fluid::Kernel::Debrun) ImGui::SetItemDefaultFocus();
        ImGui::EndCombo();
    }
    ImGui::Checkbox("Viscosity", &viscosity);
    ImGui::Checkbox("Surface Tension", &surfaceTension);

    ImGui::DragInt2("Reset Size", resetSize,1.f,1,200);
    if (ImGui::Button("Reset")) {
        playing = false;
        if (fluid) delete fluid;
        fluid = new Fluid(resetSize[0],resetSize[1]);
    }

    ImGui::End();
}

void FluidSimulator::imGuiDisplay() {
    showMenuBar();

    if (showInfo)
        showInfoGUI();
    if (showControls)
        showControlsGUI();
}

string FluidSimulator::validateName(string filename) {
    if (filename.find(".") == string::npos) {
        return filename + ".obj";
    }
    else if (filename.substr(filename.length() - 4) == ".obj") {
        return filename;
    }
    else {
        return "";
    }
}