#include "vendors/imgui/imgui.h"
#include "vendors/imgui/imgui_impl_opengl3.h"
#include "vendors/imgui/imgui_impl_glfw.h"
//#include "vendors/BVH.h"
#include "BVH.h"

#include "GL/glew.h"
#include "GLFW/glfw3.h"

#include <glm/matrix.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>

#include "BVHEditor.h"

#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>

using namespace std;

BVHEditor::BVHEditor() {
    if (!init()) {}
    //newBVH test("files/02_01.bvh");
    //cout << test.loaded << endl;

}

void BVHEditor::cleanup() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
}

// initialise GLFW, glew and ImGui
int BVHEditor::init() {
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
    camDir = glm::vec3{ 0.f,0.f,-1.f};

    frameNo = 0;
    currFrameTime = 0.;
    prevFrameTime = 0.;

    desiredFrameTime = 0.;

    return 1;
}

void BVHEditor::run() {
    mainLoop();
    cleanup();
}

void BVHEditor::mainLoop() {
    prevFrameTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        // Reset frame timer
        //glfwSetTime(0);

        // Handle input
        glfwPollEvents();
        if (!showSaveDialog and !showLoadDialog)
            keyboardCheck();

        // ------- OpenGL -------

        // Clear Screen
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        // Render animation
        if (currBVH != NULL) {
            currBVH->forwardKin(frameNo, &(currBVH->jPos));
            currBVH->RenderJoints(&(currBVH->jPos));
        }

        // Set camera properties
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(fov, 1, 0.1, 100);

        // Set camera position and direction
        if (autoTarget and currBVH != NULL) {
            glm::vec3 skelRoot(currBVH->getMotion(frameNo, 0), currBVH->getMotion(frameNo, 1), currBVH->getMotion(frameNo, 2));
            camDir = skelRoot - camPos;
            camDir = glm::normalize(camDir);
            gluLookAt(camPos.x, camPos.y, camPos.z, skelRoot.x, skelRoot.y, skelRoot.z, 0, 1, 0);

        }
        else {
            glm::vec3 camTarget = camPos + camDir;
            gluLookAt(camPos.x, camPos.y, camPos.z, camTarget.x, camTarget.y, camTarget.z, 0, 1, 0);
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

        if (currBVH != NULL && playing) {
            frameNo =(frameNo + 1) % currBVH->frameCount;
        }

        // Calculate frame timings and sleep to hit desired framerate
        float frameTimeRemaining = desiredFrameTime - (glfwGetTime() - currFrameTime);
        if (frameTimeRemaining > 0) {
            this_thread::sleep_for(chrono::milliseconds((int)(frameTimeRemaining * 1000)));
        }
        prevFrameTime = currFrameTime;
        currFrameTime = glfwGetTime();
        deltaFrameTime = currFrameTime - prevFrameTime;

    }
}

void BVHEditor::keyboardCheck() {
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
        glm::vec3 rightVec = glm::rotateY(camDir, -1*glm::half_pi<float>());
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
        camDir = glm::rotate(camDir, -1 * camSpeed,leftVec);
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
        camDir = glm::rotateY(camDir, -1* camSpeed);
    }
}

void BVHEditor::mouseCallback(int button, int action, int mods) {
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

        // Find the vector pointing to that point
        crVec = glm::normalize(projClick - camPos);
        crOri = camPos;

        //cout << distanceToClick(glm::vec3()) << endl;
        if (currBVH)
            selectJoint();
    }
}

void BVHEditor::selectJoint() {
    float shortD=FLT_MAX;
    int shortID=-1;
    for (size_t i = 0; i < currBVH->jPos.size(); i++) {
        glm::vec3 p = currBVH->jPos[i];
        float d = distanceToClick(p);
        if (d < shortD) {
            shortD = d;
            shortID = i;
        }
    }
    if (shortD < 0.01) {
        if (selectedJoint != -1) currBVH->joints[selectedJoint]->selected = false;
        selectedJoint = shortID;
        currBVH->joints[selectedJoint]->selected = true;
        string n = currBVH->joints[selectedJoint]->name;
        glm::vec3 p = currBVH->jPos[selectedJoint];
        std::cout << "Selected("<<selectedJoint<<"): " << n << " at " << p.x << "," << p.y << "," << p.z << std::endl;
        targetP[0] = p.x;
        targetP[1] = p.y;
        targetP[2] = p.z;
    }
}

void BVHEditor::showMenuBar() {
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
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    showFileDialog();
}

void BVHEditor::showFileDialog() {
    if (showLoadDialog) {
        ImGui::Begin("Open .bvh file");
        ImGui::Text("Enter Filename: ");
        ImGui::SameLine();
        ImGui::InputText("", fnbuf, 256);
        ImGui::Spacing();
        ImGui::SameLine();
        if (ImGui::Button("Confirm")) {
            if (!loadBVH(fnbuf)) {
                showFNFModal = true;
            }
            else {
                showLoadDialog = false;
                memset(fnbuf, 0, sizeof fnbuf);
                showControls = true;
            }
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
    } else if (showSaveDialog) {
        ImGui::Begin("Save .bvh file");
        ImGui::Text("Enter Filename: ");
        ImGui::SameLine();
        ImGui::InputText("", fnbuf, 256);
        ImGui::Spacing();
        ImGui::SameLine();
        if (ImGui::Button("Confirm")) {
            if (!writeBVHFile(fnbuf)) {
                showSaveErrorModal = true;
            }
            else {
                showSaveDialog = false;
                memset(fnbuf, 0, sizeof fnbuf);
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

void BVHEditor::showInfoGUI() {
    ImGui::Begin("Info");
    if (currBVH != NULL) {
        ImGui::Text("Animation Name: %s", currBVH->motionName.c_str());
    }
    else {
        ImGui::Text("Animation Name: No animation loaded");
    }
    int framerate = floor(1.0 / (double)deltaFrameTime);
    ImGui::Text("Framerate: %i", framerate);
    ImGui::Text("Frame Time: %f", deltaFrameTime);
    ImGui::End();
}

void BVHEditor::showControlsGUI() {
    ImGui::Begin("Controls");
    if (currBVH == NULL) {
        ImGui::Text("No animation loaded");
    }
    else {
        ImGui::Text("Animation Controls");
        // Play/Pause Button
        if (playing) {
            if (ImGui::Button("Pause")) {
                playing = false;
            }
        }
        else {
            if (ImGui::Button("Play")) {
                playing = true;
            }
        }

        // Frame Select Slider
        ImGui::SliderInt("Frame Select", &frameNo, 0, currBVH->frameCount);
        ImGui::Text("Camera Controls");
        ImGui::Checkbox("Target Skeleton", &autoTarget);
        ImGui::SliderFloat("Camera Sensitivity", &camSpeed, 0.01f, 0.1f);
        ImGui::SliderFloat("Movement Speed", &moveSpeed, 0.1f, 1.f);
        if (ImGui::Button("Move Point")) {
            if (currBVH) {
                // Do the inverse kinematics
                if (selectedJoint != -1) {
                    currBVH->inverseKin(glm::vec3(targetP[0], targetP[1], targetP[2]), frameNo, selectedJoint);
                }
            }
        }
        ImGui::InputFloat("x", &targetP[0]);
        ImGui::InputFloat("y", &targetP[1]);
        ImGui::InputFloat("z", &targetP[2]);
        if (ImGui::Button("Move Points")) {
            if (currBVH) {
                if (currBVH->joints.size() > 27) {
                    vector<glm::vec3> p; vector<int> i;
                    p.push_back(glm::vec3(6, 8., 0.));
                    p.push_back(glm::vec3(-11.7, 5.45, -0.64));
                    i.push_back(20);
                    i.push_back(27);
                    currBVH->inverseKin(p, frameNo, i);
                }
            }
        }
    }
    ImGui::End();
}

void BVHEditor::imGuiDisplay() {
    showMenuBar();

    if (showInfo)
        showInfoGUI();
    if (showControls)
        showControlsGUI();
}

bool BVHEditor::loadBVH(string filename) {
    currBVH = new BVH(("files/" + filename).c_str());
    if (currBVH->loaded) {
        frameNo = 0;
        desiredFrameTime = currBVH->interval;
        return 1;
    }
    currBVH = NULL;
    return 0;
}

bool BVHEditor::writeBVHFile(string filename) {
    if (currBVH == NULL) {
        saveErrorContents = "No BVH loaded to save.";
        return false;
    }

    filename = validateName(filename);

    if (filename == "") {
        saveErrorContents = "File must be a .bvh file to save";
        return false;
    }

    ofstream file(("files/" + filename).c_str(),ofstream::trunc);
    if (file.is_open()) {
        file.precision(6);
        file << "HIERARCHY" << endl;

        BVH::Joint *rootJ = const_cast<BVH::Joint *>(currBVH->joints[0]);

        file << "ROOT ";

        writeJoint(rootJ, &file);

        file << "MOTION" << endl;

        writeMotion(&file);

        file.close();
    }
    else {
        saveErrorContents = "File could not be opened";
        return false;
    }
    return true;
}

void BVHEditor::writeMotion( ofstream* file) {
    *file << "Frames: " << currBVH->frameCount << endl;
    *file << "Frame Time: " << currBVH->interval << endl;

    for (size_t fi = 0; fi < currBVH->frameCount; fi++) {
        for (size_t ci = 0; ci < currBVH->channelCount; ci++) {
            *file << currBVH->getMotion(fi, ci) << (ci < currBVH->channelCount - 1? " " : "");
        }
        *file << endl;
    }
}


void BVHEditor::writeJoint(BVH::Joint* j, ofstream* file) {
    // Write name
    *file << j->name << endl << "{" << endl;

    // Write offsets
    glm::vec3 off = j->offset;
    *file << "OFFSET " << fixed << off.x << " " << fixed << off.y << " " << fixed << off.z << endl;

    // Write channels
    vector<BVH::Channel *> ch = j->channels;
    *file << "CHANNELS " << ch.size();

    for (size_t i = 0; i < ch.size(); i++) {
        *file << " ";

        BVH::ChannelEnum type = ch[i]->type;

        switch (type) {
        case BVH::ChannelEnum::X_ROTATION: *file << "Xrotation"; break;
        case BVH::ChannelEnum::Y_ROTATION: *file << "Yrotation"; break;
        case BVH::ChannelEnum::Z_ROTATION: *file << "Zrotation"; break;
        case BVH::ChannelEnum::X_POSITION: *file << "Xposition"; break;
        case BVH::ChannelEnum::Y_POSITION: *file << "Yposition"; break;
        case BVH::ChannelEnum::Z_POSITION: *file << "Zposition"; break;
        }
    }
    *file << endl;

    if (j->hasSite) {
        glm::vec3 siteoff = j->site;
        *file << "End Site" << endl << "{" << endl;
        *file << "OFFSET " << fixed << siteoff.x << " " << fixed << siteoff.y << " " << fixed << siteoff.z << endl;
        *file << "}" << endl;
    }
    else if (j->children.size() != 0 ) {
        for (size_t i = 0; i < j->children.size(); i++) {
            *file << "JOINT ";
            writeJoint(j->children[i], file);
        }
    }
    *file << "}" << endl;
}

string BVHEditor::validateName(string filename) {
    if (filename.find(".") == string::npos) {
        return filename + ".bvh";
    }
    else if (filename.substr(filename.length() - 4) == ".bvh") {
        return filename;
    }
    else {
        return "";
    }
}

float BVHEditor::distanceToClick(glm::vec3 p) {
    return glm::angle(crVec, glm::normalize(p - crOri));
}