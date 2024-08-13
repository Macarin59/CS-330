#include <iostream>         // cout, cerr
#include <cstdlib>          // EXIT_FAILURE
#include <GL/glew.h>        // GLEW library
#include <GLFW/glfw3.h>     // GLFW library
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"     // Image loading Utility functions

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "meshes.h"
#include "camera.h"

using namespace std; // Standard namespace

/*Shader program Macro*/
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

namespace
{
    const char* const WINDOW_TITLE = "Macarin Final Project"; // Macro for window title

    // Variables for window width and height
    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 600;

    // Stores the GL data relative to a given mesh
    struct GLMesh
    {
        GLuint vao;         // Vertex Array Object
        GLuint vbos[2];     // Vertex buffer objects
        GLuint nIndices;    // Number of indices
    };

    // Main GLFW window
    GLFWwindow* gWindow = nullptr;
    // Shader program
    //GLuint gProgramId;
    GLuint gCubeProgramId;
    GLuint gLampProgramId;
    // Shape Meshes from Professor Battersby
    Meshes meshes;
    // Texture id
    GLuint gTextureId1, gTextureId2, gTextureId3, gTextureId4, gTextureId5, gTextureId6;
    glm::vec2 gUVScale(1.0f, 3.0f);
    GLint gTexWrapMode = GL_REPEAT;

    // camera
    Camera gCamera(glm::vec3(0.0f, 8.3f, 7.3f));
    float gLastX = WINDOW_WIDTH / 2.0f;
    float gLastY = WINDOW_HEIGHT / 2.0f;
    bool gFirstMouse = true;

    // timing
    float gDeltaTime = 0.0f; // time between current frame and last frame
    float gLastFrame = 0.0f;

    bool isOrtho = false; // boolean to handle ortho checks
    float inputDelay = 0.0f; // input delay for perspective change

    // Light information
    glm::vec3 gLightColor(1.0f, 0.84f, 0.67f);
    glm::vec3 gLightPosition(20.0f, 20.0f, 20.0f);
    glm::vec3 gLightScale(0.0f);
}

// Function declarations
bool UInitialize(int, char* [], GLFWwindow** window);
void UResizeWindow(GLFWwindow* window, int width, int height);
void UProcessInput(GLFWwindow* window);
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void URender();
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId);
void UDestroyShaderProgram(GLuint programId);
bool UCreateTexture(const char* filename, GLuint& textureId);
void UDestroyTexture(GLuint textureId);

/* Cube Vertex Shader Source Code*/
const GLchar* cubeVertexShaderSource = GLSL(440,

    layout(location = 0) in vec3 position; // VAP position 0 for vertex position data
layout(location = 1) in vec3 normal; // VAP position 1 for normals
layout(location = 2) in vec2 textureCoordinate;

out vec3 vertexNormal; // For outgoing normals to fragment shader
out vec3 vertexFragmentPos; // For outgoing color / pixels to fragment shader
out vec2 vertexTextureCoordinate;

//Uniform / Global variables for the  transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates

    vertexFragmentPos = vec3(model * vec4(position, 1.0f)); // Gets fragment / pixel position in world space only (exclude view and projection)

    vertexNormal = mat3(transpose(inverse(model))) * normal; // get normal vectors in world space only and exclude normal translation properties
    vertexTextureCoordinate = textureCoordinate;
}
);


/* Cube Fragment Shader Source Code*/
const GLchar* cubeFragmentShaderSource = GLSL(440,

    in vec3 vertexNormal; // For incoming normals
in vec3 vertexFragmentPos; // For incoming fragment position
in vec2 vertexTextureCoordinate;

out vec4 fragmentColor; // For outgoing cube color to the GPU

// Uniform / Global variables for object color, light color, light position, and camera/view position
uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 lightColor2;
uniform vec3 lightPos;
uniform vec3 lightPos2;
uniform vec3 viewPosition;
uniform sampler2D uTexture; // Useful when working with multiple textures
uniform vec2 uvScale;

void main()
{
    /*Phong lighting model calculations to generate ambient, diffuse, and specular components*/

    //Calculate Ambient lighting*/
    float ambientStrength = 1.0f; // Set ambient or global lighting strength
    vec3 ambient = ambientStrength * lightColor; // Generate ambient light color
    float ambientStrength2 = 0.1f; // Set ambient or global lighting strength
    vec3 ambient2 = ambientStrength2 * lightColor2; // Generate ambient light color

    //Calculate Diffuse lighting*/
    vec3 norm = normalize(vertexNormal); // Normalize vectors to 1 unit
    vec3 lightDirection = normalize(lightPos - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
    float impact = max(dot(norm, lightDirection), 0.0);// Calculate diffuse impact by generating dot product of normal and light
    vec3 diffuse = impact * lightColor; // Generate diffuse light color

    vec3 lightDirection2 = normalize(lightPos2 - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
    float impact2 = max(dot(norm, lightDirection2), 0.0);// Calculate diffuse impact by generating dot product of normal and light
    vec3 diffuse2 = impact2 * lightColor2; // Generate diffuse light color

    //Calculate Specular lighting*/
    float specularIntensity = 0.8f; // Set specular light strength
    float highlightSize = 16.0f; // Set specular highlight size
    float specularIntensity2 = 0.8f; // Set specular light strength
    float highlightSize2 = 16.0f; // Set specular highlight size

    vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
    vec3 reflectDir = reflect(-lightDirection, norm);// Calculate reflection vector
    vec3 reflectDir2 = reflect(-lightDirection2, norm);// Calculate reflection vector

    //Calculate specular component
    float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
    vec3 specular = specularIntensity * specularComponent * lightColor;

    float specularComponent2 = pow(max(dot(viewDir, reflectDir2), 0.0), highlightSize2);
    vec3 specular2 = specularIntensity2 * specularComponent2 * lightColor2;

    // Texture holds the color to be used for all three components
    vec4 textureColor = texture(uTexture, vertexTextureCoordinate * uvScale);

    // Calculate phong result
    vec3 phong = (ambient + diffuse + specular) * textureColor.xyz;
    vec3 phong2 = (ambient2 + diffuse2 + specular2) * textureColor.xyz;

    fragmentColor = vec4(phong + phong2, 1.0); // Send lighting results to GPU
}
);


/* Lamp Shader Source Code*/
const GLchar* lampVertexShaderSource = GLSL(440,

    layout(location = 0) in vec3 position; // VAP position 0 for vertex position data

//Uniform / Global variables for the  transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates
}
);


/* Fragment Shader Source Code*/
const GLchar* lampFragmentShaderSource = GLSL(440,

    out vec4 fragmentColor; // For outgoing lamp color (smaller cube) to the GPU

void main()
{
    fragmentColor = vec4(1.0f); // Set color to white (1.0f,1.0f,1.0f) with alpha 1.0
}
);
/* Old Shader Code
/* Vertex Shader Source Code
const GLchar* vertexShaderSource = GLSL(440,
    layout(location = 0) in vec3 position;
    layout(location = 2) in vec2 textureCoordinate;

    out vec2 vertexTextureCoordinate;


    //Global variables for the transform matrices
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    void main()
    {
        gl_Position = projection * view * model * vec4(position, 1.0f); // transforms vertices to clip coordinates
        vertexTextureCoordinate = textureCoordinate;
    }
);


/* Fragment Shader Source Code
const GLchar* fragmentShaderSource = GLSL(440,
    in vec2 vertexTextureCoordinate;

    out vec4 fragmentColor;

    uniform sampler2D uTexture;

    void main()
    {
        fragmentColor = texture(uTexture, vertexTextureCoordinate); // Sends texture to the GPU for rendering
    }
); */

// Images are loaded with Y axis going down, but OpenGL's Y axis goes up, so let's flip it
void flipImageVertically(unsigned char* image, int width, int height, int channels)
{
    for (int j = 0; j < height / 2; ++j)
    {
        int index1 = j * width * channels;
        int index2 = (height - 1 - j) * width * channels;

        for (int i = width * channels; i > 0; --i)
        {
            unsigned char tmp = image[index1];
            image[index1] = image[index2];
            image[index2] = tmp;
            ++index1;
            ++index2;
        }
    }
}

int main(int argc, char* argv[]) {
    if (!UInitialize(argc, argv, &gWindow)) {
        return EXIT_FAILURE;
    }

    meshes.CreateMeshes(); // Creates the mesh

    // Create the shader programs
    if (!UCreateShaderProgram(cubeVertexShaderSource, cubeFragmentShaderSource, gCubeProgramId))
        return EXIT_FAILURE;

    if (!UCreateShaderProgram(lampVertexShaderSource, lampFragmentShaderSource, gLampProgramId))
        return EXIT_FAILURE;

    // Load texture 1
    const char* texFilename = "ceramic.jpg";
    if (!UCreateTexture(texFilename, gTextureId1))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    // Load texture 2
    texFilename = "wood.jpg";
    if (!UCreateTexture(texFilename, gTextureId2))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    // Load texture 3
    texFilename = "metallic purple.jpg";
    if (!UCreateTexture(texFilename, gTextureId3))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    // Load texture 4
    texFilename = "black.jpg";
    if (!UCreateTexture(texFilename, gTextureId4))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    // Load texture 5
    texFilename = "blue.jpg";
    if (!UCreateTexture(texFilename, gTextureId5))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    // Load texture 6
    texFilename = "card.jpg";
    if (!UCreateTexture(texFilename, gTextureId6))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }

    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    glUseProgram(gCubeProgramId);
    // We set the texture as texture unit 0
    glUniform1i(glGetUniformLocation(gCubeProgramId, "uTexture"), 0);
    
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Clears background color

    while (!glfwWindowShouldClose(gWindow)) { // Render loop
        float currentFrame = glfwGetTime();
        gDeltaTime = currentFrame - gLastFrame;
        gLastFrame = currentFrame;
        inputDelay -= gDeltaTime;

        UProcessInput(gWindow); // Input
        URender(); // Render frame
        glfwPollEvents();
    }

    meshes.DestroyMeshes(); // Release mesh data

    // Release textures
    UDestroyTexture(gTextureId1); 
    UDestroyTexture(gTextureId2); 
    UDestroyTexture(gTextureId3);
    UDestroyTexture(gTextureId4);
    UDestroyTexture(gTextureId5);
    UDestroyTexture(gTextureId6);
    
    // Release shader program
    UDestroyShaderProgram(gCubeProgramId);
    UDestroyShaderProgram(gLampProgramId);

    exit(EXIT_SUCCESS); // Terminates the program successfully
}


// Initialize GLFW, GLEW, and create a window
bool UInitialize(int argc, char* argv[], GLFWwindow** window) {
    // GLFW: initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    #ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

    // GLFW: window creation
    * window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
    if (*window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(*window);
    glfwSetFramebufferSizeCallback(*window, UResizeWindow);
    glfwSetCursorPosCallback(*window, UMousePositionCallback);
    glfwSetScrollCallback(*window, UMouseScrollCallback);
    glfwSetMouseButtonCallback(*window, UMouseButtonCallback);

    // tell GLFW to capture our mouse
    //glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // GLEW: initialize
    // Note: if using GLEW version 1.13 or earlier
    glewExperimental = GL_TRUE;
    GLenum GlewInitResult = glewInit();

    if (GLEW_OK != GlewInitResult) {
        std::cerr << glewGetErrorString(GlewInitResult) << std::endl;
        return false;
    }

    // Displays GPU OpenGL version
    cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << endl;

    return true;
}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void UProcessInput(GLFWwindow* window) {
    static const float cameraSpeed = 2.5f;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Conditional to disable controls in orthographic view
    if (!isOrtho) {
        // Keyboard inputs
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            gCamera.ProcessKeyboard(FORWARD, gDeltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            gCamera.ProcessKeyboard(BACKWARD, gDeltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            gCamera.ProcessKeyboard(LEFT, gDeltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            gCamera.ProcessKeyboard(RIGHT, gDeltaTime);
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
            gCamera.ProcessKeyboard(UP, gDeltaTime);
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
            gCamera.ProcessKeyboard(DOWN, gDeltaTime);
    }
    
    // Orthographic input
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
        if (inputDelay <= 0) {
            if (!isOrtho) {
                isOrtho = true;
            }
            else {
                isOrtho = false;
            }
            inputDelay = 0.25f;
        }
    }
}


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void UResizeWindow(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}


// Functioned called to render a frame
void URender() {
    glm::mat4 scale;
    glm::mat4 rotation;
    glm::mat4 translation;
    glm::mat4 model;
    glm::mat4 mainModel; // Allows for complex objects
    glm::mat4 view;
    glm::mat4 projection;
    GLint modelLoc;
    GLint viewLoc;
    GLint projLoc;
    GLint objectColorLoc;

    // Enable z-depth
    glEnable(GL_DEPTH_TEST);

    // Changes to wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // Clear the frame and z buffers
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!isOrtho) {
        view = gCamera.GetViewMatrix();
        projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
    }
    else {
        view = glm::translate(glm::vec3(0.0f, 0.0f, 0.0f));
        projection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 100.0f);
    }

    /*
    * Object: Table
    */

    // Set the shader to be used
    glUseProgram(gCubeProgramId);

    // Retrieves and passes transform matrices to the Shader program
    modelLoc = glGetUniformLocation(gCubeProgramId, "model");
    viewLoc = glGetUniformLocation(gCubeProgramId, "view");
    projLoc = glGetUniformLocation(gCubeProgramId, "projection");
    //objectColorLoc = glGetUniformLocation(gCubeProgramId, "uObjectColor");

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(meshes.gPlaneMesh.vao); // Handle of cup

    // 1. Scales the object
    scale = glm::scale(glm::vec3(10.0f, 10.0f, 10.0f));
    // 2. Rotate the object
    rotation = glm::rotate(0.0f, glm::vec3(1.0, 1.0f, 1.0f));
    // 3. Position the object
    translation = glm::translate(glm::vec3(0.0f, -1.0f, 0.0f));
    // Model matrix: transformations are applied right-to-left order
    model = translation * rotation * scale;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // Reference matrix uniforms from the Cube Shader program for the cub color, light color, light position, and camera position
    objectColorLoc = glGetUniformLocation(gCubeProgramId, "objectColor");
    GLint lightColorLoc = glGetUniformLocation(gCubeProgramId, "lightColor");
    GLint lightPositionLoc = glGetUniformLocation(gCubeProgramId, "lightPos");
    GLint viewPositionLoc = glGetUniformLocation(gCubeProgramId, "viewPosition");

    // Pass color, light, and camera data to the Cube Shader program's corresponding uniforms
    glUniform3f(objectColorLoc, 1.0f, 1.0f, 1.0f);
    glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
    glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);
    const glm::vec3 cameraPosition = gCamera.Position;
    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    GLint UVScaleLoc = glGetUniformLocation(gCubeProgramId, "uvScale");
    glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));

    glProgramUniform4f(gCubeProgramId, objectColorLoc, 1.0f, 1.0f, 0.0f, 1.0f);

    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureId2);

    // Draws the triangles
    glDrawElements(GL_TRIANGLES, meshes.gPlaneMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);

    /*
    * Object: Cup
    */
    
    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(meshes.gTaperedCylinderMesh.vao); // Main body of cup

    // 1. Scales the object
    scale = glm::scale(glm::vec3(1.0f, 1.0f, 1.0f));
    // 2. Rotate the object
    rotation = glm::rotate(3.14159f, glm::vec3(2.0, 0.0f, 1.0f));
    // 3. Position the object
    translation = glm::translate(glm::vec3(-2.0f, 0.01f, 0.0f));
    // Model matrix: transformations are applied right-to-left order
    mainModel = translation * rotation * scale;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(mainModel));

    glProgramUniform4f(gCubeProgramId, objectColorLoc, 0.0f, 1.0f, 1.0f, 1.0f);

    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureId1);

    // Draws the triangles
    //glDrawArrays(GL_TRIANGLE_FAN, 0, 36);		//bottom
    glDrawArrays(GL_TRIANGLE_FAN, 36, 36);		//top
    glDrawArrays(GL_TRIANGLE_STRIP, 72, 146);	//sides

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);

    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(meshes.gTorusMesh.vao); // Handle of cup

    // 1. Scales the object
    scale = glm::scale(glm::vec3(0.3f, 0.4f, 1.5f));
    // 2. Rotate the object
    rotation = glm::rotate(0.25f, glm::vec3(0.0, 0.0f, 0.18f));
    // 3. Position the object
    translation = glm::translate(glm::vec3(1.05f, 0.6f, 0.0f));
    // Model matrix: transformations are applied right-to-left order
    model = mainModel * translation * rotation * scale;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    glProgramUniform4f(gCubeProgramId, objectColorLoc, 0.0f, 0.0f, 1.0f, 1.0f);

    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureId1);

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, meshes.gTorusMesh.nVertices);

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);

    /*
    * Object: Tissue Box
    */

    glBindVertexArray(meshes.gBoxMesh.vao);

    // 1. Scales the object
    scale = glm::scale(glm::vec3(4.0f, 1.5f, 2.0f));
    // 2. Rotate the object
    rotation = glm::rotate(3.14159f, glm::vec3(0.0, 0.0f, 1.0f));
    // 3. Position the object
    translation = glm::translate(glm::vec3(2.0f, -0.248f, 0.0f));
    // Model matrix: transformations are applied right-to-left order
    model = translation * rotation * scale;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    glProgramUniform4f(gCubeProgramId, objectColorLoc, 0.0f, 1.0f, 1.0f, 1.0f);

    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureId5);

    glDrawElements(GL_TRIANGLES, meshes.gBoxMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);

    /*
    * Object: Metal cup
    */

    glBindVertexArray(meshes.gCylinderMesh.vao); // Body of metal cup

    // 1. Scales the object
    scale = glm::scale(glm::vec3(0.7f, 3.5f, 0.7f));
    // 2. Rotate the object
    rotation = glm::rotate(3.14159f, glm::vec3(0.0, 0.0f, 1.0f));
    // 3. Position the object
    translation = glm::translate(glm::vec3(-4.0f, 2.525f, 0.0f));
    // Model matrix: transformations are applied right-to-left order
    mainModel = translation * rotation * scale;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(mainModel));

    glProgramUniform4f(gCubeProgramId, objectColorLoc, 0.0f, 1.0f, 1.0f, 1.0f);

    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureId3);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 36);		//bottom
    glDrawArrays(GL_TRIANGLE_FAN, 36, 36);		//top
    glDrawArrays(GL_TRIANGLE_STRIP, 72, 146);	//sides

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);

    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(meshes.gCylinderMesh.vao); // Straw of metal cup

    // 1. Scales the object
    scale = glm::scale(glm::vec3(0.08f, 1.0f, 0.08f));
    // 2. Rotate the object
    rotation = glm::rotate(3.14159f, glm::vec3(0.0, 0.0f, 1.0f));
    // 3. Position the object
    translation = glm::translate(glm::vec3(0.0f, 0.6f, 0.0f));
    // Model matrix: transformations are applied right-to-left order
    model = mainModel * translation * rotation * scale;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    glProgramUniform4f(gCubeProgramId, objectColorLoc, 0.0f, 0.0f, 1.0f, 1.0f);

    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureId4);

    glDrawArrays(GL_TRIANGLE_STRIP, 72, 146);	//sides

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);

    /*
    * Object: Stack of cards
    */

    glBindVertexArray(meshes.gPlaneMesh.vao); // First card/base of stack

    // 1. Scales the object
    scale = glm::scale(glm::vec3(0.5f, 1.0f, 0.8f));
    // 2. Rotate the object
    rotation = glm::rotate(3.14159f, glm::vec3(0.0, 0.0f, 1.0f));
    // 3. Position the object
    translation = glm::translate(glm::vec3(5.5f, -0.99f, 0.0f));
    // Model matrix: transformations are applied right-to-left order
    mainModel = translation * rotation * scale;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(mainModel));

    glDrawElements(GL_TRIANGLES, meshes.gPlaneMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureId6);

    glDrawElements(GL_TRIANGLES, meshes.gBoxMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);

    for (int i = 1; i <= 20; ++i) {
        glBindVertexArray(meshes.gPlaneMesh.vao);

        // 1. Scales the object
        scale = glm::scale(glm::vec3(1.0f, 1.0f, 1.0f));
        // 2. Rotate the object
        rotation = glm::rotate(3.14159f, glm::vec3(0.0, 0.0f, 1.0f));
        // 3. Position the object
        translation = glm::translate(glm::vec3(0.0f, i*(-0.005f), 0.0f));
        // Model matrix: transformations are applied right-to-left order
        model = mainModel * translation * rotation * scale;
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        glDrawElements(GL_TRIANGLES, meshes.gPlaneMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

        // bind textures on corresponding texture units
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gTextureId6);

        glDrawElements(GL_TRIANGLES, meshes.gBoxMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

        // Deactivate the Vertex Array Object
        glBindVertexArray(0);
    }

    // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
    glfwSwapBuffers(gWindow);    // Flips the the back buffer with the front buffer every frame.
}

// Implements the UCreateShaders function
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId) {
    // Compilation and linkage error reporting
    int success = 0;
    char infoLog[512];

    // Create a Shader program object.
    programId = glCreateProgram();

    // Create the vertex and fragment shader objects
    GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

    // Retrive the shader source
    glShaderSource(vertexShaderId, 1, &vtxShaderSource, NULL);
    glShaderSource(fragmentShaderId, 1, &fragShaderSource, NULL);

    // Compile the vertex shader, and print compilation errors (if any)
    glCompileShader(vertexShaderId); // compile the vertex shader
    // check for shader compile errors
    glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glCompileShader(fragmentShaderId); // compile the fragment shader
    // check for shader compile errors
    glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShaderId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    // Attached compiled shaders to the shader program
    glAttachShader(programId, vertexShaderId);
    glAttachShader(programId, fragmentShaderId);

    glLinkProgram(programId);   // links the shader program
    // check for linking errors
    glGetProgramiv(programId, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glUseProgram(programId);    // Uses the shader program

    return true;
}


void UDestroyShaderProgram(GLuint programId) {
    glDeleteProgram(programId);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (!isOrtho) {
        if (gFirstMouse)
        {
            gLastX = xpos;
            gLastY = ypos;
            gFirstMouse = false;
        }

        float xoffset = xpos - gLastX;
        float yoffset = gLastY - ypos; // reversed since y-coordinates go from bottom to top

        gLastX = xpos;
        gLastY = ypos;

        gCamera.ProcessMouseMovement(xoffset, yoffset);
    }
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (!isOrtho) {
        gCamera.ProcessMouseScroll(yoffset);
    }
}

// glfw: handle mouse button events
// --------------------------------
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (!isOrtho) {
        switch (button)
        {
        case GLFW_MOUSE_BUTTON_LEFT:
        {
            if (action == GLFW_PRESS)
                cout << "Left mouse button pressed" << endl;
            else
                cout << "Left mouse button released" << endl;
        }
        break;

        case GLFW_MOUSE_BUTTON_MIDDLE:
        {
            if (action == GLFW_PRESS)
                cout << "Middle mouse button pressed" << endl;
            else
                cout << "Middle mouse button released" << endl;
        }
        break;

        case GLFW_MOUSE_BUTTON_RIGHT:
        {
            if (action == GLFW_PRESS)
                cout << "Right mouse button pressed" << endl;
            else
                cout << "Right mouse button released" << endl;
        }
        break;

        default:
            cout << "Unhandled mouse button event" << endl;
            break;
        }
    }
}

/*Generate and load the texture*/
bool UCreateTexture(const char* filename, GLuint& textureId)
{
    int width, height, channels;
    unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);
    if (image)
    {
        flipImageVertically(image, width, height, channels);

        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);

        // set the texture wrapping parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // set texture filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        if (channels == 3)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        else if (channels == 4)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        else
        {
            cout << "Not implemented to handle image with " << channels << " channels" << endl;
            return false;
        }

        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(image);
        glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

        return true;
    }

    // Error loading the image
    return false;
}


void UDestroyTexture(GLuint textureId)
{
    glGenTextures(1, &textureId);
}