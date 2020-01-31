#include <GL/gl3w.h>
#include <GLFW/glfw3.h> // GLFW helper library
#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <iostream>

// IMGUI
#include <imgui.h>
#include <imgui_impl_glfw_gl3.h>

#include "Shaders.h"
#include "Object.h"

#define PI 3.141592f
inline float DEG2RAD(float deg) { return (PI * deg / 180.0f); }
inline float RAD2DEG(float rad) { return (rad * (180.0f / PI)); }

/*---------------------------- Variables ----------------------------*/
// GLFW window
GLFWwindow* window;
int width = 1280;
int height = 720;

GLuint shader_program, star_shader_program;
glm::mat4 viewMatrix;
glm::mat4 projectionMatrix;

glm::vec4 lightPos = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
glm::vec4 lightCol = glm::vec4(1.0f, 1.0f, 1.0f, 10.0f);
bool isPointLight = false;

// 3D models
Model stars;
glm::vec3   accumPos = glm::vec3(0.0f);

#define DRAW_STARS

// Spaceship
Model spaceship[6];
int currentShip = 0;

const float spaceshipMaxSpeed = 5.0f;

glm::vec2   spaceshipVelocity = glm::vec2(0.0f); // We're only moving X and Y
glm::vec3   spaceshipPosition = glm::vec3(0.0f);

float       spaceshipAngularVelocity = 0.0f;
float       spaceshipRotation = 0.0f;


/*---------------------------- Functions ----------------------------*/
void Initialize()
{
    // Create a shader for the lab
    GLuint vs = buildShader(GL_VERTEX_SHADER, ASSETS"basic.vert");
    GLuint fs = buildShader(GL_FRAGMENT_SHADER, ASSETS"diffuse.frag");
    shader_program = buildProgram(vs, fs, 0);

    vs = buildShader(GL_VERTEX_SHADER, ASSETS"fullscreen.vert");
    fs = buildShader(GL_FRAGMENT_SHADER, ASSETS"fullscreen.frag");
    star_shader_program = buildProgram(vs, fs, 0);

    // We need to bind the locations before linking the program
    glBindAttribLocation(shader_program, VERTEX_LOC, "vPosition");
    glBindAttribLocation(shader_program, NORMAL_LOC, "vNormal");
    glBindAttribLocation(shader_program, COLORS_LOC, "vColor");

    // Link and dump the shader errors etc
    linkProgram(shader_program);
    dumpProgram(shader_program, "Diffuse Lighting shader program");

    glBindAttribLocation(star_shader_program, VERTEX_LOC, "vPosition");

    linkProgram(star_shader_program);
    dumpProgram(star_shader_program, "Star shader program");

    // Load in the models
    for (int i = 0; i < 6; i++)
    {
        std::string spaceshipNumber = "spaceCraft" + std::to_string(i + 1) + ".obj";
        spaceship[i] = Load3DModel(ASSETS"Models/", spaceshipNumber);
    }

    {
        float positions[12] =
        {
            -1.0f, -1.0f, 0.0f,
            -1.0f,  1.0f, 0.0f,
             1.0f, -1.0f, 0.0f,
             1.0f,  1.0f, 0.0f
        };

        glGenVertexArrays(1, &stars.vao);
        glBindVertexArray(stars.vao);

        glGenBuffers(1, &stars.vbo);
        glBindBuffer(GL_ARRAY_BUFFER, stars.vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 12, &positions[0], GL_STATIC_DRAW);

        // Vertex info
        glVertexAttribPointer(VERTEX_LOC, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glEnableVertexAttribArray(VERTEX_LOC);

        // Uncomment the line below when you've fixed the code above
        stars.vertexCount = (GLuint)4;
    }

}

void Update(float deltaTime)
{
    //-------------------------------------SPACESHIP MOVEMENT-----------------------------------//
    // Very simple spaceship movement, get the keyboard input
    if (glfwGetKey(window, GLFW_KEY_LEFT))  spaceshipAngularVelocity += DEG2RAD(5.0f) * deltaTime; // 180 deg per second
    if (glfwGetKey(window, GLFW_KEY_RIGHT)) spaceshipAngularVelocity -= DEG2RAD(5.0f) * deltaTime; // 180 deg per second
    if (glfwGetKey(window, GLFW_KEY_DOWN))
    {
        spaceshipVelocity -= glm::vec2(-sin(spaceshipRotation), cos(spaceshipRotation)) * 0.5f * deltaTime;
    }
    if (glfwGetKey(window, GLFW_KEY_UP))
    {
        spaceshipVelocity += glm::vec2(-sin(spaceshipRotation), cos(spaceshipRotation)) * 0.5f * deltaTime;
    }

    // Enforce maximum speed here
    if (glm::length(spaceshipVelocity) > spaceshipMaxSpeed)
    {
        spaceshipVelocity = glm::normalize(spaceshipVelocity) * spaceshipMaxSpeed;
    }
    // And maximum rotation here
    if (spaceshipAngularVelocity > DEG2RAD(720.0f)) // If we're spinning more than 2 times per second
    {
        spaceshipAngularVelocity = DEG2RAD(720.0f);
    }

    // Move our spaceship
    spaceshipPosition.x += spaceshipVelocity.x;
    spaceshipPosition.y += spaceshipVelocity.y;
    accumPos.x += spaceshipVelocity.x;
    accumPos.y += spaceshipVelocity.y;

    // Rotate our spaceship
    spaceshipRotation += spaceshipAngularVelocity;

    spaceshipVelocity *= 0.99f; // Simple friction coefficient so we slow down
    spaceshipAngularVelocity *= 0.99f; // Simple angular friction coefficient so we stop spinning

    //-------------------------------------PROJECTION MATRICES-----------------------------------//

    viewMatrix = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 10.0f),   // Where the camera is in 3D space
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f));   // Which way is up in the world

    float ratio = width / (float)height;

    //projectionMatrix = glm::perspective(DEG2RAD(90.0f), ratio, 0.1f, 100.0f);
	projectionMatrix = glm::ortho(-10.0f * ratio, 10.0f * ratio, -10.0f, 10.0f, -100.0f, 100.0f);

    // Because the orthographic matrix vertically goes from -10 to 10, we can wrap the spaceship Y position here
    if (spaceshipPosition.y > 10.0f)    spaceshipPosition.y = -10.0f;
    if (spaceshipPosition.y < -10.0f)   spaceshipPosition.y = 10.0f;

    // Because the orthographic matrix horizontally goes from -10 * ratio to 10 * ratio, we can wrap the spaceship Y position here
    if (spaceshipPosition.x > 10.0f * ratio)    spaceshipPosition.x = -10.0f * ratio;
    if (spaceshipPosition.x < -10.0f * ratio)   spaceshipPosition.x = 10.0f * ratio;
}

void RenderModel(glm::mat4 modelMatrix, Model model)
{
    glm::mat4 normalMat = glm::inverse(glm::transpose(viewMatrix * modelMatrix));
    glm::mat4 modelViewProjMat = projectionMatrix * viewMatrix * modelMatrix;

    GLuint normal_loc = glGetUniformLocation(shader_program, "normalMat");
    glUniformMatrix4fv(normal_loc, 1, 0, &normalMat[0][0]);
    GLuint model_loc = glGetUniformLocation(shader_program, "modelMat");
    glUniformMatrix4fv(model_loc, 1, 0, &modelMatrix[0][0]);


    glBindVertexArray(model.vao);
    glDrawArrays(GL_TRIANGLES, 0, model.vertexCount);
}

void Render()
{
    // Draw the stars
    
#ifdef DRAW_STARS
    glUseProgram(star_shader_program);
    glBindVertexArray(stars.vao);
    glUniform1f(glGetUniformLocation(star_shader_program, "ratio"), (float)height / (float)width);
    glUniform1f(glGetUniformLocation(star_shader_program, "iTime"), (float)glfwGetTime());
    glUniform2fv(glGetUniformLocation(star_shader_program, "position"), 1, &accumPos[0]);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, stars.vertexCount);
#endif

    // Draw the spaceship
    glUseProgram(shader_program);

    // Set view projection matrix
    GLuint vp_loc = glGetUniformLocation(shader_program, "viewProjMat");
    glUniformMatrix4fv(vp_loc, 1, 0, &(projectionMatrix * viewMatrix)[0][0]);

    // Set lighting information
    GLuint light_loc = glGetUniformLocation(shader_program, "lightPosDir");
    GLuint color_loc = glGetUniformLocation(shader_program, "lightColor"); // RGB is color, A is intensity
    glUniform4fv(light_loc, 1, &lightPos[0]);
    glUniform4fv(color_loc, 1, &lightCol[0]);

    {   // Draw the spaceship
        glm::mat4 modelMat = glm::mat4(1.0f);
        modelMat = glm::translate(modelMat, spaceshipPosition);
        modelMat = glm::rotate(modelMat, spaceshipRotation, glm::vec3(0.0f, 0.0f, 1.0f));
        modelMat = glm::rotate(modelMat, 90.0f, glm::vec3(1.0f, 0.0f, 0.0f));
        modelMat = glm::scale(modelMat, glm::vec3(0.1f));
        RenderModel(modelMat, spaceship[currentShip]);
    }
}

void Cleanup()
{
    for (int i = 0; i < 6; i++)
    {
        // cleanup the spaceship
        glDeleteBuffers(1, &spaceship[i].vbo);
        glDeleteVertexArrays(1, &spaceship[i].vao);
    }

    glUseProgram(GL_NONE);
    glDeleteProgram(shader_program);
}

void GUI()
{
    bool isWindowOpen = true;
    ImGui::Begin("Settings", &isWindowOpen, ImVec2(), 0.3f);
    {
        // Show some basic stats in the settings window 
        ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

        ImGui::RadioButton("Spaceship 1", &currentShip, 0);
        ImGui::RadioButton("Spaceship 2", &currentShip, 1);
        ImGui::RadioButton("Spaceship 3", &currentShip, 2);
        ImGui::RadioButton("Spaceship 4", &currentShip, 3);
        ImGui::RadioButton("Spaceship 5", &currentShip, 4);
        ImGui::RadioButton("Spaceship 6", &currentShip, 5);
    }
    ImGui::End();
}

static void ResizeEvent(GLFWwindow* a_window, int a_width, int a_height)
{
    // Set the viewport incase the window size changed
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
}

int main()
{
    // start GL context and O/S window using the GLFW helper library
    if (!glfwInit())
    {
        fprintf(stderr, "ERROR: could not start GLFW3\n");
        return 1;
    }

    window = glfwCreateWindow(width, height, "Laboratory 4", NULL, NULL);
    if (!window)
    {
        fprintf(stderr, "ERROR: could not open window with GLFW3\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSetWindowSizeCallback(window, ResizeEvent);

    glfwSwapInterval(1);

    // start GL3W
    gl3wInit();

    // Setup ImGui binding. This is for any parameters you want to control in runtime
    ImGui_ImplGlfwGL3_Init(window, true);
    ImGui::StyleColorsLight();

    // Get version info
    const GLubyte* renderer = glGetString(GL_RENDERER); // get renderer string
    const GLubyte* version = glGetString(GL_VERSION); // version as a string
    printf("Renderer: %s\n", renderer);
    printf("OpenGL version supported %s\n", version);

    glDisable(GL_DEPTH_TEST); // disable depth-testing

    Initialize();

    float lastFrameTime = 0.0f;
    while (!glfwWindowShouldClose(window))
    {
        float currentFrameTime = (float)glfwGetTime();
        float deltaTime = currentFrameTime - lastFrameTime;
        lastFrameTime = currentFrameTime;

        // update other events like input handling 
        glfwPollEvents();

        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        ImGui_ImplGlfwGL3_NewFrame();

        // Call the helper functions
        Update(deltaTime);
        Render();
        GUI();

        // Finish by drawing the GUI on top of everything
        ImGui::Render();
        glfwSwapBuffers(window);
    }

    // close GL context and any other GLFW resources
    glfwTerminate();
    ImGui_ImplGlfwGL3_Shutdown();
    Cleanup();
    return 0;
}