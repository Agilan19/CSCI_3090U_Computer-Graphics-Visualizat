#include <GL/gl3w.h>
#include <GLFW/glfw3.h> // GLFW helper library
#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <iostream>

// IMGUI
#include <imgui.h>
#include <imgui_impl_glfw_gl3.h>

#include "Shaders.h"
#include "mesh.h"

#define PI 3.141592

#define VERTEX_LOC      0
#define NORMAL_LOC      1
#define TEXCOORD_LOC    2

using namespace glm;

inline float DEG2RAD(float deg) { return (PI * deg / 180.0f); }
inline float RAD2DEG(float rad) { return (rad * (180.0 / PI)); }

/*---------------------------- Variables ----------------------------*/
// GLFW window
GLFWwindow* window;
int width = 1280;
int height = 720;

GLuint shader_program, unlit_shader_program;
mat4 viewMatrix;
mat4 projectionMatrix;

struct Light
{
    vec3 lightPos;
    vec3 lightDir;
    vec4 lightColor;
    int lightType;
    float lightAngle;
};

const int NUM_LIGHTS = 5;

Light lights[NUM_LIGHTS] =
{
    {vec3(0), vec3(0, -1, 0), vec4(1,0,0,4), 1, cos(DEG2RAD(45.0f))},
    {vec3(0), vec3(0, -1, 0), vec4(0,1,0,4), 1, cos(DEG2RAD(45.0f))},
    {vec3(0), vec3(0, -1, 0), vec4(0,1,1,4), 1, cos(DEG2RAD(45.0f))},
    {vec3(0), vec3(0, -1, 0), vec4(1,1,0,4), 1, cos(DEG2RAD(45.0f))},
    {vec3(0), vec3(0, -1, 0), vec4(0,0,1,4), 1, cos(DEG2RAD(45.0f))}
};

vec3 cameraPosition = vec3(0, 2, -10);

struct Model
{
    GLuint vao;
    GLuint vbo; // interleaved. This means vertex, normal, and colors are all in one

    unsigned int vertexCount;
};

/*---------------------------- Functions ----------------------------*/

void Initialize()
{
    // Create a shader for the ground
    {
        GLuint vs = buildShader(GL_VERTEX_SHADER, ASSETS"basic.vert");
        GLuint fs = buildShader(GL_FRAGMENT_SHADER, ASSETS"diffuse.frag");
        shader_program = buildProgram(vs, fs, 0);

        // We need to bind the locations before linking the program
        glBindAttribLocation(shader_program, VERTEX_LOC, "vPosition");
        glBindAttribLocation(shader_program, NORMAL_LOC, "vNormal");
    }

    // Create a shader for the lights (unlit)
    {
        GLuint vs = buildShader(GL_VERTEX_SHADER, ASSETS"basic.vert");
        GLuint fs = buildShader(GL_FRAGMENT_SHADER, ASSETS"unlit.frag");
        unlit_shader_program = buildProgram(vs, fs, 0);

        // We need to bind the locations before linking the program
        glBindAttribLocation(unlit_shader_program, VERTEX_LOC, "vPosition");
        glBindAttribLocation(unlit_shader_program, NORMAL_LOC, "vNormal");
    }


    // Link and dump the shader errors etc
    linkProgram(shader_program);
    dumpProgram(shader_program, "Multiple Lights shader program");
    linkProgram(unlit_shader_program);
    dumpProgram(unlit_shader_program, "Unlit shader program");
}

void Update()
{
    const float pi2 = 3.1415f * 2.0f;
    const float seperation = pi2 / (float)NUM_LIGHTS;

    if (NUM_LIGHTS < 3) return; // just in case

    for (int i = 0; i < 5; i++)
    {
		// rotates the lights
        lights[i].lightPos.x = cos(glfwGetTime() + (i * seperation));
        lights[i].lightPos.y = 1.0f;
        lights[i].lightPos.z = sin(glfwGetTime() + (i * seperation));
    }

}

void Render()
{
    glUseProgram(shader_program);

    viewMatrix = lookAt( cameraPosition, vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
    projectionMatrix = perspective(DEG2RAD(50.0f), width / (float)height, 0.01f, 100.0f);

    // Set view projection matrix
    GLuint vp_loc = glGetUniformLocation(shader_program, "viewProjMat");
    glUniformMatrix4fv(vp_loc, 1, 0, &(projectionMatrix * viewMatrix)[0][0]);

    // Set lighting information
    GLuint loc_lightPos = glGetUniformLocation(shader_program, "lightPos");
    GLuint loc_lightDir = glGetUniformLocation(shader_program, "lightDir");
    GLuint loc_lightColor = glGetUniformLocation(shader_program, "lightColor"); // RGB is color, A is intensity
    GLuint loc_spotAngle = glGetUniformLocation(shader_program, "spotAngle");
    GLuint loc_lightType = glGetUniformLocation(shader_program, "lightType");
    GLuint loc_numLights = glGetUniformLocation(shader_program, "numLights");
    GLuint loc_cameraPos = glGetUniformLocation(shader_program, "cameraPos");

    // We need each light parameter to be in an array now, because we send up arrays
    vec3  lightPositions[NUM_LIGHTS], lightDirections[NUM_LIGHTS];
    vec4  lightColors[NUM_LIGHTS];
    float lightAngles[NUM_LIGHTS];
    int   lightType[NUM_LIGHTS];

    // Add it here
	for (int i = 0; i < NUM_LIGHTS; i++) {
		lightPositions[i] = lights[i].lightPos;
		lightDirections[i] = lights[i].lightDir;
		lightColors[i] = lights[i].lightColor;
		lightAngles[i] = lights[i].lightAngle;
		lightType[i] = lights[i].lightType;
	}

    // Here's where we set each variable in the shader. The second parameter is how many items we're sending up
    glUniform3fv(loc_lightPos,      NUM_LIGHTS, &lightPositions[0][0]);
    glUniform3fv(loc_lightDir,      NUM_LIGHTS, &lightDirections[0][0]);
    glUniform4fv(loc_lightColor,    NUM_LIGHTS, &lightColors[0][0]);
    glUniform1fv(loc_spotAngle,     NUM_LIGHTS, &lightAngles[0]);
    glUniform1iv(loc_lightType,     NUM_LIGHTS, &lightType[0]);      

    //  And we let the shader know how many lights there are
    glUniform1i(loc_numLights, NUM_LIGHTS);

    glUniform3fv(loc_cameraPos, 1, &cameraPosition[0]);
    {   // Draw the Ground
        mat4 modelMat = mat4(1.0f);
        modelMat = scale(modelMat, vec3(200.0f, 0.1f, 200.0f));

        GLuint model_loc = glGetUniformLocation(shader_program, "modelMat");
        glUniformMatrix4fv(model_loc, 1, 0, &modelMat[0][0]);

        Primitive::DrawBox();
    }

    // So we populate the arrays with the values from each light
    glUseProgram(unlit_shader_program);

    vp_loc = glGetUniformLocation(unlit_shader_program, "viewProjMat");
    glUniformMatrix4fv(vp_loc, 1, 0, &(projectionMatrix * viewMatrix)[0][0]);
    loc_cameraPos = glGetUniformLocation(unlit_shader_program, "cameraPos");
    glUniform3fv(loc_cameraPos, 1, &cameraPosition[0]);

    for (int i = 0; i < NUM_LIGHTS; i++)
    {
        mat4 modelMat = mat4(1.0f);
        modelMat = translate(modelMat, lights[i].lightPos);
        modelMat = scale(modelMat, vec3(0.1f));

        GLuint model_loc = glGetUniformLocation(unlit_shader_program, "modelMat");
        glUniformMatrix4fv(model_loc, 1, 0, &(modelMat)[0][0]);
        GLuint color_loc = glGetUniformLocation(unlit_shader_program, "color");
        glUniform4fv(color_loc, 1, &lights[i].lightColor[0]);

        Primitive::DrawSphere();
    }
}

void Cleanup()
{
    glUseProgram(GL_NONE);
    glDeleteProgram(shader_program);
}

void GUI()
{
    for (int i = 0; i < NUM_LIGHTS; i++)
    {
        ImGui::Begin(("LIGHT #" + std::to_string(i)).c_str());
        {
            ImGui::ColorEdit3("Color",       &lights[i].lightColor[0]);
            ImGui::InputFloat("Intensity",   &lights[i].lightColor[3]);
            ImGui::SliderFloat("Cone Angle", &lights[i].lightAngle, 0.0f, 1.0f);
            ImGui::SliderInt("Color",        &lights[i].lightType, 0, 2);
        }
        ImGui::End();
    }
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

    window = glfwCreateWindow(width, height, "Multiple Lights Demo", NULL, NULL);
    if (!window)
    {
        fprintf(stderr, "ERROR: could not open window with GLFW3\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSetWindowSizeCallback(window, ResizeEvent);

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

    // tell GL to only draw onto a pixel if the shape is closer to the viewer
    glEnable(GL_DEPTH_TEST); // enable depth-testing
    glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"

    Initialize();

    while (!glfwWindowShouldClose(window))
    {
        // update other events like input handling 
        glfwPollEvents();

        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        ImGui_ImplGlfwGL3_NewFrame();

        // Call the helper functions
        Update();
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