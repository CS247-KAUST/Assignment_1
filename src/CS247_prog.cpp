// CS 247 - Scientific Visualization, KAUST
//
// Programming Assignment #1

#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <string>
#include <sstream>
#include <cassert>

#include <cmath>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

// framework includes
#include "glslprogram.h"

// window size
const unsigned int gWindowWidth = 512;
const unsigned int gWindowHeight = 512;

// Declarations
void loadData(char* filename);
void downloadVolumeAsTexture();

unsigned short* data_array;
unsigned short vol_dim[3];
int current_axis;
int current_slice[3];
bool data_loaded;

GLuint texture;
GLuint VBO[2], VAO, EBO;
GLSLProgram program;
glm::mat4 model;

// Cycle clear colors
static void nextClearColor(void)
{
    static int color = 0;

    switch(color++)
    {
        case 0:
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            break;
        case 1:
            glClearColor(0.2f, 0.2f, 0.4f, 1.0f);
            break;
        default:
            glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
            color = 0;
            break;
    }
}

// callbacks
void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    // TODO: fix aspect ratio when window is resized

}
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_RELEASE) {
        switch (key) {
            case GLFW_KEY_ESCAPE:
                std::cout << "Bye :D" << std::endl;
                glfwSetWindowShouldClose(window, true);
                break;
            case GLFW_KEY_W:
                current_slice[current_axis] = std::min((current_slice[current_axis] + 1),  vol_dim[current_axis] - 1);
                fprintf(stderr, "increasing current slice: %i\n", current_slice[current_axis]);
                break;
            case GLFW_KEY_S:
                current_slice[current_axis] = std::max((current_slice[current_axis] - 1), 0);
                fprintf(stderr, "decreasing current slice: %i\n", current_slice[current_axis]);
                break;
            case GLFW_KEY_A: // optional
                current_axis = ((current_axis + 1) % 3);
                fprintf(stderr, "toggling viewing axis to: %i\n", current_axis);
                break;
            case GLFW_KEY_1:
                loadData("../data/lobster.dat");
                break;
            case GLFW_KEY_2:
                loadData("../data/skewed_head.dat");
                break;
            case GLFW_KEY_B:
                nextClearColor();
                break;
            default:
                fprintf(stderr, "\nKeyboard commands:\n\n"
                                "b - Toggle among background clear colors\n"
                                "w - Increase current slice\n"
                                "s - Decrease current slice\n"
                                "a - Toggle viewing axis\n"
                                "1 - Load lobster dataset\n"
                                "2 - Load head dataset\n");
                break;

        }
    }
}


// glfw error callback
static void errorCallback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}


// Data
void loadData(char* filename)
{
    fprintf(stderr, "loading data %s\n", filename);

    FILE* fp;
    fp = fopen(filename, "rb");// open file, read only, binary mode
    if (fp == NULL) {
        fprintf(stderr, "Cannot open file %s for reading.\n", filename);
        return;
    }

//    memset(vol_dim, 0, sizeof(unsigned short) * 3);

    //read volume dimension
    fread(&vol_dim[0], sizeof(unsigned short), 1, fp);
    fread(&vol_dim[1], sizeof(unsigned short), 1, fp);
    fread(&vol_dim[2], sizeof(unsigned short), 1, fp);

    fprintf(stderr, "volume dimensions: x: %i, y: %i, z:%i \n", vol_dim[0], vol_dim[1], vol_dim[2]);

    if (data_array != NULL) {
        delete[] data_array;
    }

    data_array = new unsigned short[vol_dim[0] * vol_dim[1] * vol_dim[2]]; //for intensity volume

    for(int z = 0; z < vol_dim[2]; z++) {
        for(int y = 0; y < vol_dim[1]; y++) {
            for(int x = 0; x < vol_dim[0]; x++) {

                fread(&data_array[x + (y * vol_dim[0]) + (z * vol_dim[0] * vol_dim[1])], sizeof(unsigned short), 1, fp);
                data_array[x + (y * vol_dim[0]) + (z * vol_dim[0] * vol_dim[1])] = data_array[x + (y * vol_dim[0]) + (z * vol_dim[0] * vol_dim[1])] << 4;
            }
        }
    }

    fclose(fp);

    current_slice[0] = vol_dim[0] / 2;
    current_slice[1] = vol_dim[1] / 2;
    current_slice[2] = vol_dim[2] / 2;

    downloadVolumeAsTexture();

    data_loaded = true;
}

void downloadVolumeAsTexture()
{
    fprintf(stderr, "downloading volume to 3D texture\n");

    // TODO: Set up and download 3D texture to GPU
    // Hint: set up texture parameters

    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, vol_dim[0], vol_dim[1], vol_dim[2], 0,
                 GL_LUMINANCE, GL_UNSIGNED_SHORT, data_array);

}


// init application
// - load application specific data
// - set application specific parameters
// - initialize stuff
bool initApplication(int argc, char **argv)
{

    std::string version((const char *)glGetString(GL_VERSION));
    std::stringstream stream(version);
    unsigned major, minor;
    char dot;

    stream >> major >> dot >> minor;

    assert(dot == '.');
    if (major > 3 || (major == 2 && minor >= 0)) {
        std::cout << "OpenGL Version " << major << "." << minor << std::endl;
    } else {
        std::cout << "The minimum required OpenGL version is not supported on this machine. Supported is only " << major << "." << minor << std::endl;
        return false;
    }

    // default initialization
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glDisable(GL_DEPTH_TEST);

    // viewport
    glViewport(0, 0, gWindowWidth, gWindowHeight);

    // projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    return true;
}


// set up the scene: shaders, VAO, ..etc
void setup() {
    // compile & link the shaders
    program.compileShader("../shaders/vertex.vs");
    program.compileShader("../shaders/fragment.fs");
    program.link();

    // TODO: make slices
    // Hint: set up slice VAO, VBO and an EBO with position and texture coordinates

}

// render a frame
void render() {
    // binding texture
    glBindTexture(GL_TEXTURE_3D, texture);

    // use shader
    program.use();

    // TODO: update slices according to user preference
    // hint: change the slice and orientation based on current_axis and current_slice
    // hint2: transformation matrix applied on texture coordinate might be useful!

    // TODO: set unifroms (if any)

    // render VAO
}

// entry point
int main(int argc, char** argv)
{
    // initialize variables
    data_array = NULL;
    current_slice[0] = 0;
    current_slice[1] = 0;
    current_slice[2] = 0;
    current_axis = 2;
    data_loaded = false;


    // set glfw error callback
    glfwSetErrorCallback(errorCallback);

    // init glfw
    if (!glfwInit()) {
        exit(EXIT_FAILURE);
    }

    // init glfw window
    GLFWwindow* window;
    window = glfwCreateWindow(gWindowWidth, gWindowHeight, "CS247 - Scientific Visualization - Slice Viewer", nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    // set GLFW callback functions
    glfwSetKeyCallback(window, keyCallback);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    // make context current (once is sufficient)
    glfwMakeContextCurrent(window);

    // get the frame buffer size
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    // init the OpenGL API (we need to do this once before any calls to the OpenGL API)
    gladLoadGL();

    // init our application
    if (!initApplication(argc, argv)) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }


    // set up the scene
    setup();

    // print menu
    keyCallback(window, GLFW_KEY_BACKSLASH, 0, GLFW_PRESS, 0);

    // start traversing the main loop
    // loop until the user closes the window
    while (!glfwWindowShouldClose(window))
    {
        // clear frame buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // render one frame
        render();

        // swap front and back buffers
        glfwSwapBuffers(window);

        // poll and process input events (keyboard, mouse, window, ...)
        glfwPollEvents();
    }

    glfwTerminate();
    return EXIT_SUCCESS;
}
