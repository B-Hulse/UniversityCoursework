#include "vendors/imgui/imgui.h"
#include "vendors/imgui/imgui_impl_opengl3.h"
#include "vendors/imgui/imgui_impl_glfw.h"

//#include "GL/glew.h"
//#include "GLFW/glfw3.h"

#include <glm/matrix.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>

#include "ClothSimulator.h"
#include "vendors/lodepng/lodepng.h"

#include <iostream>
#include <fstream>
#include <chrono>
#include <stdint.h>

using namespace std;

ClothSimulator::ClothSimulator() {
    if (!init()) {}
}

void ClothSimulator::cleanup() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    if (object != NULL) delete object;
}

// initialise GLFW, glew and ImGui
int ClothSimulator::init() {
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

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    float p[4] = { 1.,0.,1.,0. };
    glLightfv(GL_LIGHT0, GL_POSITION, p);

    // Set camera properties
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(fov, 1, 0.1, 100);

    glClearColor(0.45f, 0.55f, 0.60f, 1.00f);

    // Initialise ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    // initialise ImGui inputs
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // init Camera Position
    camPos = glm::vec3{ 0.f,0.f,1.f };
    camDir = glm::vec3{ 0.f,0.f,-1.f };

    frameNo = 0;
    currFrameTime = 0.;
    prevFrameTime = 0.;

    desiredFrameTime = 0.;

    return 1;
}

void ClothSimulator::run() {
    mainLoop();
    cleanup();
}

void ClothSimulator::mainLoop() {
    prevFrameTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {

        if (lighting) glEnable(GL_LIGHTING);
        else glDisable(GL_LIGHTING);

        // Handle input
        glfwPollEvents();
        if (!showSaveDialog and !showLoadDialog)
            keyboardCheck();

        // ------- OpenGL -------

        // Clear Screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        camDir = glm::vec3() - camPos;
        camDir = glm::normalize(camDir);
        glm::vec3 camTarget = camPos + camDir;
        gluLookAt(camPos.x, camPos.y, camPos.z, camTarget.x, camTarget.y, camTarget.z, 0, 1, 0);

        // Framerate bound section
        if ((glfwGetTime() - currFrameTime) >= desiredFrameTime) {

            // Update Physics
            if (cloth != NULL) {
                if (playing) {
                    cloth->updateNorms();
                    cloth->updatePhys(sphereActive, floorActive, cornersPin, sphereSpin, sphereSpinSpeed, wind);
                }
            }

            // Get new frame timings
            prevFrameTime = currFrameTime;
            currFrameTime = glfwGetTime();
            deltaFrameTime = currFrameTime - prevFrameTime;
        }

        // Render object here
        if (cloth != NULL) {
            cloth->render(clothWireframe);
        }

        if (floorActive) {
            float l[4] = { .5f, .5f ,.5f,1.f };
            glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, l);

            // Render floor
            glNormal3f(0., 1., 0.);
            glColor3f(0.7, 0.7, 0.7);
            glBegin(GL_QUADS);
            glVertex3f(-5., 0., -5);
            glVertex3f(-5., 0., 5);
            glVertex3f(5., 0., 5);
            glVertex3f(5., 0., -5);
            glEnd();
        }

        if (sphereActive) {
            float l[4] = { .5f, .5f ,.5f,1.f };
            glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, l);
            // RenderSphere
            GLUquadricObj* quad = gluNewQuadric();
            gluQuadricDrawStyle(quad, GLU_FILL);
            gluQuadricNormals(quad, GLU_SMOOTH);
            glColor3f(0.6, 0.6, 0.6);
            glTranslatef(0., .5, 0.);
            gluSphere(quad,.45f,20,20);
            glTranslatef(0., -.5, 0.);
        }

        if (recording and playing) {
            savePNG();
            frameNo++;
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
    }
}

void ClothSimulator::savePNG() {
    if (frameNo % 4 != 0) return;
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

void ClothSimulator::keyboardCheck() {
    // Camera Position Movement
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        camPos -= camDir * moveSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        camPos += camDir * moveSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        glm::vec3 leftVec = glm::rotateY(camDir, glm::half_pi<float>());
        leftVec.y = 0;
        camPos += leftVec * moveSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        glm::vec3 rightVec = glm::rotateY(camDir, -1 * glm::half_pi<float>());
        rightVec.y = 0;
        camPos += rightVec * moveSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        glm::vec3 upVec{ 0.f,1.f,0.f };
        camPos += upVec * moveSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
        glm::vec3 downVec{ 0.f,-1.f,0.f };
        camPos += downVec * moveSpeed;
    }

    // Camera Direction Movement
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        glm::vec3 leftVec = glm::rotateY(camDir, glm::half_pi<float>());
        leftVec.y = 0;
        camDir = glm::rotate(camDir, -1 * camSpeed, leftVec);
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        glm::vec3 leftVec = glm::rotateY(camDir, glm::half_pi<float>());
        leftVec.y = 0;
        camDir = glm::rotate(camDir, camSpeed, leftVec);
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        camDir = glm::rotateY(camDir, camSpeed);
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        camDir = glm::rotateY(camDir, -1 * camSpeed);
    }
}

void ClothSimulator::mouseCallback(int button, int action, int mods) {
    // Left mouse click
    if (button == GLFW_MOUSE_BUTTON_1 and action == GLFW_PRESS) {
        // get mouse pos
        double mX, mY;
        glfwGetCursorPos(window, &mX, &mY);

        // Calculate the offset on the screen mapped between -1 and 1
        mX = (mX / ((double)WINWIDTH / 2.)) - 1;
        mY = (mY / ((double)WINHEIGHT / 2.)) - 1;

        // Calculate the intersection with the plane 1 unit from the camera
        glm::vec3 leftVec = glm::normalize(glm::vec3(-1 * camDir.z, 0.f, camDir.x));
        glm::vec3 projClick = (camPos + camDir) + ((float)mX * leftVec) + ((float)mY * glm::cross(camDir, leftVec));
    }
}

void ClothSimulator::showMenuBar() {
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            //showControlsMenuBar();
            if (ImGui::MenuItem("Save", "CTRL+S")) {
                showSaveDialog = true;
            }
            if (ImGui::MenuItem("Load", "CTRL+S")) {
                showLoadDialog = true;
            }
            if (ImGui::MenuItem("Exit")) {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::Checkbox("Info GUI", &showInfo);
            ImGui::Checkbox("Controls GUI", &showControls);
            ImGui::Checkbox("Cloth Maker", &showMakeWindow);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    showFileDialog();
}

void ClothSimulator::showFileDialog() {
    if (showLoadDialog) {
        ImGui::Begin("Open .bvh file");
        ImGui::Text("Enter Filename: ");
        ImGui::SameLine();
        ImGui::InputText("", fnbuf, 256);
        ImGui::Spacing();
        ImGui::SameLine();
        if (ImGui::Button("Confirm")) {
            // Load file here
            if (!loadCloth("files/" + string(fnbuf))) {
                showFNFModal = true;
            }
            showLoadDialog = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            showLoadDialog = false;
            memset(fnbuf, 0, sizeof fnbuf);
        }
        if (showFNFModal) {
            ImGui::OpenPopup("FNFPopup");
        }
        if (ImGui::BeginPopup("FNFPopup")) {
            ImGui::Text("File could not be opened, please try again.");
            if (ImGui::Button("Ok")) {
                showFNFModal = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::End();
        }
        ImGui::End();
    }
    else if (showSaveDialog) {
        ImGui::Begin("Save .bvh file");
        ImGui::Text("Enter Filename: ");
        ImGui::SameLine();
        ImGui::InputText("", fnbuf, 256);
        ImGui::Spacing();
        ImGui::SameLine();
        if (ImGui::Button("Confirm")) {
            // Save file here
            if (object) {
                if (!object->saveOBJ("files/" + string(fnbuf))) {
                    saveErrorContents = "File could not be saved";
                    showSaveErrorModal = true;
                }
                showSaveDialog = false;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            showSaveDialog = false;
            memset(fnbuf, 0, sizeof fnbuf);
        }
        if (showSaveErrorModal) {
            ImGui::OpenPopup("SaveErrorPopup");
        }
        if (ImGui::BeginPopup("SaveErrorPopup")) {
            ImGui::Text(saveErrorContents.c_str());
            if (ImGui::Button("Ok")) {
                showSaveErrorModal = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::End();
        }
        ImGui::End();
    }
}

void ClothSimulator::showInfoGUI() {
    ImGui::Begin("Info");
    int framerate = floor(1.0 / (double)deltaFrameTime);
    ImGui::Text("Framerate: %i", framerate);
    ImGui::Text("Frame Time: %f", deltaFrameTime);
    ImGui::Text("Frame: %i", frameNo);
    ImGui::End();
}

void ClothSimulator::showControlsGUI() {
    ImGui::Begin("Controls");
    ImGui::InputDouble("Frametime", &desiredFrameTime);
    if(ImGui::Button(playing ? "Pause" : "Play")) {
        playing = !playing;
    }
    ImGui::SameLine();
    ImGui::Checkbox("Record", &recording);
    ImGui::Checkbox("Draw Wireframe", &clothWireframe);
    ImGui::Checkbox("Floor", &floorActive);
    ImGui::Checkbox("Sphere", &sphereActive);
    ImGui::Checkbox("Sphere Spin", &sphereSpin);
    ImGui::InputFloat("Sphere SpinSpeed", &sphereSpinSpeed);
    ImGui::Checkbox("Pin Corners", &cornersPin);
    ImGui::Checkbox("Wind", &wind);
    ImGui::Checkbox("Lighting", &lighting);

    if (ImGui::Button("Print Debug")) {
        for (size_t i = 0; i < cloth->norms.size(); i++)
        {
            cout << cloth->norms[i].x << "," << cloth->norms[i].y << "," << cloth->norms[i].z << endl;
        }
    }

    ImGui::End();
}

void ClothSimulator::showMakeOBJ() {
    ImGui::Begin("Make Cloth OBJ");
    ImGui::InputFloat("Cloth Width", &makeCWidth);
    ImGui::InputInt("Cloth Division Count", &makeCDivs);
    ImGui::InputFloat("Cloth Height", &makeCHeight);
    if (ImGui::Button("Make Cloth")) {
        makeCloth();
        showMakeWindow = false;
    }
    ImGui::End();
}

void ClothSimulator::imGuiDisplay() {
    showMenuBar();

    if (showInfo)
        showInfoGUI();
    if (showControls)
        showControlsGUI();
    if (showMakeWindow)
        showMakeOBJ();
}

void ClothSimulator::makeCloth() {
    ofstream f("files/temp_cloth.obj");

    float vC = makeCDivs+1;
    float halfW = makeCWidth / 2.;

    for (size_t i = 0; i < vC; i++)
        for (size_t j = 0; j < vC; j++)
        {
            float x, y = makeCHeight, z;
            x = (i * makeCWidth / (vC - 1.f)) - halfW;
            z = (j * makeCWidth / (vC - 1.f)) - halfW;
            f << "v " << x << " " << y << " " << z << std::endl;
        }

    for (size_t i = 1; i <= (vC * vC) - vC; i++) {
        if (i % (int)vC == 0) continue;
        int j1 = i + vC;
        f << "f " << i << " " << j1 << " " << i + 1 << std::endl;
        f << "f " << i + 1 << " " << j1 << " " << j1 + 1 << std::endl;
    }
    f.close();

    loadCloth("files/temp_cloth.obj");

}

bool ClothSimulator::loadCloth(string filename) {
    if (object) delete object;
    object = new OBJ();
    if (!object->loadOBJ(filename)) {
        delete object;
        object = NULL;
        return false;
    }
    else {
        if (cloth) delete cloth;
        cloth = new Cloth();
        cloth->getMassAndSprings(object);
        showControls = true;
        frameNo = 0;
    }
    return true;
}

string ClothSimulator::validateName(string filename) {
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