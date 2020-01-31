#define GLM_ENABLE_EXPERIMENTAL
#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/matrix_access.hpp>
#include <GLM/gtx/rotate_vector.hpp>

// GLFW3
#include <GL/gl3w.h>
#include <GLFW/glfw3.h> // GLFW helper library

// IMGUI
#include <imgui.h>
#include <imgui_impl_glfw_gl3.h>

#include <iostream> // Used for 'cout'
#include <vector>   // Used for 'vector<vec3>'
#include <map>      // Used for std::map

#include "shaders.h"
#include "mesh.h"

#include <SOIL.h>

using namespace glm;

/*---------------------------- Variables ----------------------------*/
// GLFW window
GLFWwindow* window;
int width, height;

// Uniform locations for matrices
std::map<GLuint, GLuint> m_loc, v_loc, p_loc;
// textures
int currentSkybox = 0;
GLuint skybox_textures[2];
vec3 lightDirections[2] =   // These match the skyboxes
{
    normalize(vec3(-15.5f, 5.25f, -5.0f)),
    normalize(vec3(13.9662, 11.3477, -5.69199))
};

// Shader programs
GLuint simpleProgram, skyboxProgram;


// model, view, and projection matrices
mat4 m, v, vNoPos, p; vec3 camPos;

// ImGUI variables
bool isOpen = false;

// Options we can change for the lighting
float specularPower = 10.0f;
float ambientLevel = 0.2f;
vec3 tintColor = vec3(1.0f, 1.0f, 1.0f);
bool drawCube = false;

void Initialize()
{
    // Make a simple shader for the sphere we're drawing
    {
        GLuint vs = buildShader(GL_VERTEX_SHADER, ASSETS"simpleLights.vert");
        GLuint fs = buildShader(GL_FRAGMENT_SHADER, ASSETS"simpleLights.frag");
        simpleProgram = buildProgram(vs, fs, 0);
        simpleProgram = linkProgram(simpleProgram);
        dumpProgram(simpleProgram, "Simple program to test reflections");
    }

    // Make a simple shader for the skybox
    {
        GLuint vs = buildShader(GL_VERTEX_SHADER, ASSETS"skybox.vert");
        GLuint fs = buildShader(GL_FRAGMENT_SHADER, ASSETS"skybox.frag");
        skyboxProgram = buildProgram(vs, fs, 0);
        skyboxProgram = linkProgram(skyboxProgram);
        dumpProgram(skyboxProgram, "Simple program for the skybox");
    }

    m_loc[simpleProgram] = glGetUniformLocation(simpleProgram, "m");
    v_loc[simpleProgram] = glGetUniformLocation(simpleProgram, "v");
    p_loc[simpleProgram] = glGetUniformLocation(simpleProgram, "p");

    m_loc[skyboxProgram] = glGetUniformLocation(skyboxProgram, "m");
    v_loc[skyboxProgram] = glGetUniformLocation(skyboxProgram, "v");
    p_loc[skyboxProgram] = glGetUniformLocation(skyboxProgram, "p");

    skybox_textures[0] = SOIL_load_OGL_cubemap
    (
        ASSETS"skybox/cloudtop_ft.png", // xneg
        ASSETS"skybox/cloudtop_bk.png", // xpos
        ASSETS"skybox/cloudtop_up.png", // ypos
        ASSETS"skybox/cloudtop_dn.png", // yneg
        ASSETS"skybox/cloudtop_rt.png", // zpos
        ASSETS"skybox/cloudtop_lf.png", // zneg
        SOIL_LOAD_RGB,
        SOIL_CREATE_NEW_ID,
        SOIL_FLAG_MIPMAPS
    );

    skybox_textures[1] = SOIL_load_OGL_cubemap
    (
        ASSETS"skybox/alps_ft.png", // xneg
        ASSETS"skybox/alps_bk.png", // xpos
        ASSETS"skybox/alps_up.png", // ypos
        ASSETS"skybox/alps_dn.png", // yneg
        ASSETS"skybox/alps_rt.png", // zpos
        ASSETS"skybox/alps_lf.png", // zneg
        SOIL_LOAD_RGB,
        SOIL_CREATE_NEW_ID,
        SOIL_FLAG_MIPMAPS
    );

    v = inverse(lookAt(vec3(0, 1, -3), vec3(0), vec3(0, 1, 0)));
}

void Update(float deltaTime)
{
    // Set the viewport incase the window size changed
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
    float ratio = width / (float)height;

    m = mat4(1.0f);
    p = perspective(radians(60.0f), ratio, 0.1f, 1000.0f);
}

void Render()
{
    //------------------------------------------------------------------------------------------------ Draw Sky
    // Like a painter does, we draw the sky first, and then draw things on top of it.
    glUseProgram(skyboxProgram);
    
    glUniform1i(glGetUniformLocation(skyboxProgram, "skybox"), 0);
    glActiveTexture(GL_TEXTURE0);  
    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox_textures[currentSkybox]); // bind the cubemap here
    
    glUniformMatrix4fv(m_loc[skyboxProgram], 1, GL_FALSE, &m[0][0]);
    glUniformMatrix4fv(v_loc[skyboxProgram], 1, GL_FALSE, &inverse(vNoPos)[0][0]);
    glUniformMatrix4fv(p_loc[skyboxProgram], 1, GL_FALSE, &p[0][0]);
    
    Primitive::DrawSkybox();

    //------------------------------------------------------------------------------------------------ Draw Models
    // We can draw things like regular now, on top of the sky
    glUseProgram(simpleProgram);

    glUniform1i(glGetUniformLocation(simpleProgram, "skybox"), 0);
    glUniform3fv(glGetUniformLocation(simpleProgram, "cameraPosition"), 1, &camPos[0]);
    glUniform3fv(glGetUniformLocation(simpleProgram, "lightDir"), 1, &lightDirections[currentSkybox][0]);
    glUniform3fv(glGetUniformLocation(simpleProgram, "tintColor"), 1, &tintColor[0]);
    glUniform1f(glGetUniformLocation(simpleProgram, "specularPower"), pow(2.0f, specularPower));
    glUniform1f(glGetUniformLocation(simpleProgram, "ambientLevel"), ambientLevel);
    glUniformMatrix4fv(m_loc[simpleProgram], 1, GL_FALSE, &m[0][0]);
    glUniformMatrix4fv(v_loc[simpleProgram], 1, GL_FALSE, &inverse(v)[0][0]);
    glUniformMatrix4fv(p_loc[simpleProgram], 1, GL_FALSE, &p[0][0]);

    if (drawCube)   Primitive::DrawBox();
    else            Primitive::DrawSphere();

    glBindTexture(GL_TEXTURE_CUBE_MAP, GL_NONE);    // un-bind the cubemap here
}

void Cleanup()
{
    glDeleteProgram(skyboxProgram);
    glDeleteProgram(simpleProgram);
}

void GUI()
{
    // 1. Show a simple window, with the title settings
    ImGui::Begin("Lab 7 Settings", &isOpen);
    {
        ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

        ImGui::RadioButton("Clouds", &currentSkybox, 0); ImGui::SameLine();
        ImGui::RadioButton("Alps", &currentSkybox, 1);

        ImGui::Text("Specular Power = %.1f", pow(2.0f, specularPower));
        ImGui::SliderFloat("Specular Power Exp", &specularPower, 0.0f, 10.0f, "%.1f");
        ImGui::SliderFloat("Ambient Level", &ambientLevel, 0.0f, 1.0f, "%.2f");
        ImGui::ColorEdit3("Tint Color", &tintColor[0]);

        ImGui::Checkbox("Draw Cube", &drawCube);
    }
    ImGui::End();
}

void FreeCam(float deltaTime);

int main()
{
    // start GL context and O/S window using the GLFW helper library
    if (!glfwInit()) {
        fprintf(stderr, "ERROR: could not start GLFW3\n");
        return 1;
    }

    window = glfwCreateWindow(1280, 720, "Laboratory 7", NULL, NULL);
    if (!window) {
        fprintf(stderr, "ERROR: could not open window with GLFW3\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0); // essentially turn off vsync

    // start GL3W
    gl3wInit();

    // Setup ImGui binding
    ImGui_ImplGlfwGL3_Init(window, true);
    ImGui::StyleColorsLight();

    // get version info
    const GLubyte* renderer = glGetString(GL_RENDERER); // get renderer string
    const GLubyte* version = glGetString(GL_VERSION); // version as a string
    printf("Renderer: %s\n", renderer);
    printf("OpenGL version supported %s\n", version);

    // tell GL to only draw onto a pixel if the shape is closer to the viewer
    glEnable(GL_DEPTH_TEST); // enable depth-testing
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    glDisable(GL_CULL_FACE);
    glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"

    Initialize();

    float oldTime = 0.0f, currentTime = 0.0f, deltaTime = 0.0f;
    while (!glfwWindowShouldClose(window))
    {
        do { currentTime = (float)glfwGetTime(); } while (currentTime - oldTime < 1.0f / 120.0f);

        FreeCam(deltaTime);

        // update other events like input handling 
        glfwPollEvents();
        
        // Clear the screen
        glClearColor(0.96f, 0.97f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui_ImplGlfwGL3_NewFrame();

        deltaTime = currentTime - oldTime; // Difference in time
        oldTime = currentTime;
        
        // Call the helper functions
        Update(deltaTime);
        Render();
        GUI();

        // Finish by drawing the GUI
        ImGui::Render();
        glfwSwapBuffers(window);
    }

    // close GL context and any other GLFW resources
    glfwTerminate();
    ImGui_ImplGlfwGL3_Shutdown();
    Cleanup();
    return 0;
}

void FreeCam(float deltaTime)
{
    using namespace glm;

    auto left = vec3(column(v, 0));
    auto up = vec3(column(v, 1));
    auto forward = vec3(column(v, 2));
    camPos = vec3(column(v, 3));

    if (glfwGetKey(window, GLFW_KEY_W)) // Move forward
        camPos -= forward * deltaTime * 4.0f;
    if (glfwGetKey(window, GLFW_KEY_S)) // Move backward
        camPos += forward * deltaTime * 4.0f;
    if (glfwGetKey(window, GLFW_KEY_A)) // Move Left
        camPos -= left * deltaTime * 4.0f;
    if (glfwGetKey(window, GLFW_KEY_D)) // Move right
        camPos += left * deltaTime * 4.0f;

    if (glfwGetKey(window, GLFW_KEY_LEFT))
    {
        forward = rotate(forward, deltaTime, vec3(0.0f, 1.0f, 0.0f));
        left = rotate(left, deltaTime, vec3(0.0f, 1.0f, 0.0f));
        up = rotate(up, deltaTime, vec3(0.0f, 1.0f, 0.0f));
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT))
    {
        forward = rotate(forward, -deltaTime, vec3(0.0f, 1.0f, 0.0f));
        left = rotate(left, -deltaTime, vec3(0.0f, 1.0f, 0.0f));
        up = rotate(up, -deltaTime, vec3(0.0f, 1.0f, 0.0f));
    }
    if (glfwGetKey(window, GLFW_KEY_UP))
    {
        forward = rotate(forward, deltaTime, left);
        up = rotate(up, deltaTime, left);
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN))
    {
        forward = rotate(forward, -deltaTime, left);
        up = rotate(up, -deltaTime, left);
    }

    if (glfwGetKey(window, GLFW_KEY_P))
        std::cout << "Camera Position: " << camPos.x << ", " << camPos.y << ", " << camPos.z << std::endl;

    v = mat4(
        left.x,     left.y,     left.z,     0.0f,  // Col 1
        up.x,       up.y,       up.z,       0.0f,  // Col 2
        forward.x,  forward.y,  forward.z,  0.0f,  // Col 3
        camPos.x,   camPos.y,   camPos.z,   1.0f); // Col 4

    vNoPos = mat4(
        left.x,     left.y,     left.z,     0.0f,  // Col 1
        up.x,       up.y,       up.z,       0.0f,  // Col 2
        forward.x,  forward.y,  forward.z,  0.0f,  // Col 3
        0.0f,       0.0f,       0.0f,       1.0f); // Col 4
}