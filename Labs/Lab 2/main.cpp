#include <GL\gl3w.h>
#include <GLFW/glfw3.h> // GLFW helper library
#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>

// IMGUI
#include <imgui.h>
#include <imgui_impl_glfw_gl3.h>

#include <iostream> // Used for 'cout'
#include <stdio.h>  // Used for 'printf'

#include "Shaders.h"

#define PI 3.141592
inline float DEG2RAD(float deg) { return (PI * deg / 180.0f); }
inline float RAD2DEG(float rad) { return (rad * (180.0 / PI)); }

/*---------------------------- Variables ----------------------------*/
// GLFW window
GLFWwindow* window;
int width = 1280;
int height = 720;

// Uniform locations for matrices
GLuint normal_loc;
GLuint mvp_loc;

// Bunny shader program
GLuint shader_program;

// Bunny vao
GLuint cube_vbo; // vertex buffer object
GLuint cube_ebo; // element buffer object
GLuint cube_vao; // vertex array object
GLuint cube_vertexCount;

glm::mat4 modelMatrix;
glm::mat4 viewMatrix;
glm::mat4 projectionMatrix;

/*---------------------------- Functions ----------------------------*/
void CheckShader(GLuint a_shader)
{
    GLint isCompiled = 0;
    glGetShaderiv(a_shader, GL_COMPILE_STATUS, &isCompiled);
    if (isCompiled == GL_FALSE)
    {
        GLint maxLength = 0;
        glGetShaderiv(a_shader, GL_INFO_LOG_LENGTH, &maxLength);

        // The maxLength includes the NULL character
        char* errorLog = static_cast<char*>(malloc(sizeof(char) * maxLength));
        glGetShaderInfoLog(a_shader, maxLength, &maxLength, &errorLog[0]);

        printf("ERR: %s\n", errorLog);
        return;
    }
}

void Initialize()
{
    // Create a shader for the lab
    GLuint vs = buildShader(GL_VERTEX_SHADER, ASSETS"lab1.vs");
    GLuint fs = buildShader(GL_FRAGMENT_SHADER, ASSETS"lab1.fs");
    shader_program = buildProgram(vs, fs, 0);
    dumpProgram(shader_program, "Lab 1 shader program");

    // Create all 8 vertices of the cube
    glm::vec3 p0 = glm::vec3(-1.0f, -1.0f, 1.0f);
    glm::vec3 p1 = glm::vec3(1.0f, -1.0f, 1.0f);
    glm::vec3 p2 = glm::vec3(1.0f, -1.0f, -1.0f);
    glm::vec3 p3 = glm::vec3(-1.0f, -1.0f, -1.0f);
    glm::vec3 p4 = glm::vec3(-1.0f, 1.0f, 1.0f);
    glm::vec3 p5 = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec3 p6 = glm::vec3(1.0f, 1.0f, -1.0f);
    glm::vec3 p7 = glm::vec3(-1.0f, 1.0f, -1.0f);

    // Create a list of vertices (each face has 4)
    glm::vec3 vertices[24] =
    {
        // Bottom face
        p0, p1, p2, p3,
        // Left face
        p7, p4, p0, p3,
        // Front face
        p4, p5, p1, p0,
        // Back face
        p6, p7, p3, p2,
        // Right face
        p5, p6, p2, p1,
        // Top face
        p7, p6, p5, p4
    };


    // Create all 6 possible surface normal directions
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 down = glm::vec3(0.0f, -1.0f, 0.0f);
    glm::vec3 front = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 back = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 left = glm::vec3(-1.0f, 0.0f, 0.0f);
    glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f);

    // Create a list of normals (each face has 4)
    glm::vec3 normals[24] =
    {
        // Bottom face
        down, down, down, down,
        // Left face
        left, left, left, left,
        // Front face
        front, front, front, front,
        // Back face
        back, back, back, back,
        // Right face
        right, right, right, right,
        // Top face
        up, up, up, up
    };

    // Create the indices, telling OpenGL which vertex/normal to use for
    // each triangle. Indices are in groups of 3, and each face has 2 triangles.
    int triangles[36] =
    {
        // Bottom
        3, 1, 0, 3, 2, 1, // First triangle is vertex 3, 1, and 0. Second is 3, 2, and 1
                          // Left
                          7, 5, 4, 7, 6, 5,
                          // Front
                          11, 9, 8, 11, 10, 9,
                          // Back
                          15, 13, 12, 15, 14, 13,
                          // Right
                          19, 17, 16, 19, 18, 17,
                          // Top
                          23, 21, 20, 23, 22, 21
    };

    cube_vertexCount = 36;

    glGenVertexArrays(1, &cube_vao);
    glBindVertexArray(cube_vao);

    glGenBuffers(1, &cube_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, cube_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices) + sizeof(normals), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(vertices), sizeof(normals), normals);

    glGenBuffers(1, &cube_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cube_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(triangles), triangles, GL_STATIC_DRAW);

    glUseProgram(shader_program);
    GLuint vPosition = glGetAttribLocation(shader_program, "vPosition");
    glVertexAttribPointer(vPosition, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(vPosition);
    GLuint vNormal = glGetAttribLocation(shader_program, "vNormal");
    glVertexAttribPointer(vNormal, 3, GL_FLOAT, GL_FALSE, 0, (void*) sizeof(vertices));
    glEnableVertexAttribArray(vNormal);
}

void Update()
{

}

void Render()
{
    glUseProgram(shader_program);

    viewMatrix = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 10.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f));
    float ratio = width / (float)height;
    projectionMatrix = glm::perspective(DEG2RAD(50.0f), ratio, 0.01f, 100.0f);

    {   
        /*
        Create a transformation matrix here. It is used to create a larger matrix
        called the Model-View-Projection matrix. To move the cube around, we only
        change the Model matrix (aka, our transformation matrix).
        */
		glm::mat4 identity = glm::mat4(1.0f);
		glm::mat4 translation = glm::translate(identity, glm::vec3(3.0f, 0.0f, 1.0f));
		glm::mat4 scaling = glm::scale(identity, glm::vec3(0.5f, 0.5f, 0.5f));
		glm::mat4 rotation = glm::rotate(identity, 10.0f, glm::vec3(DEG2RAD(45.0f), 0.0f, 0.0f));

		// Draw a single cube
		glm::mat4 modelMatrix = translation * rotation * scaling;


        glm::mat4 normalMat = glm::inverse(glm::transpose(viewMatrix * modelMatrix));
        glm::mat4 modelViewProjMat = projectionMatrix * viewMatrix * modelMatrix;
			
        normal_loc = glGetUniformLocation(shader_program, "normalMat");
        glUniformMatrix4fv(normal_loc, 1, 0, &normalMat[0][0]);
        mvp_loc = glGetUniformLocation(shader_program, "modelViewProjMat");
        glUniformMatrix4fv(mvp_loc, 1, 0, &modelViewProjMat[0][0]);

        glBindVertexArray(cube_vao);
        glDrawElements(GL_TRIANGLES, cube_vertexCount, GL_UNSIGNED_INT, NULL);
    }

	{   
		/*
		Create a transformation matrix here. It is used to create a larger matrix
		called the Model-View-Projection matrix. To move the cube around, we only
		change the Model matrix (aka, our transformation matrix).
		*/
		glm::mat4 identity = glm::mat4(1.0f);
		glm::mat4 translation = glm::translate(identity, glm::vec3(-3.0f, sin(glfwGetTime()), 1.0f));
		glm::mat4 scaling = glm::scale(identity, glm::vec3(1.5f, 1.5f, 1.5f));
		glm::mat4 rotation = glm::rotate(identity, 10.0f, glm::vec3(0.0, 0.0f, DEG2RAD(30.0f)));

		// Draw a single cube
		glm::mat4 modelMatrix = translation * rotation * scaling;

		glm::mat4 normalMat = glm::inverse(glm::transpose(viewMatrix * modelMatrix));
		glm::mat4 modelViewProjMat = projectionMatrix * viewMatrix * modelMatrix;

		normal_loc = glGetUniformLocation(shader_program, "normalMat");
		glUniformMatrix4fv(normal_loc, 1, 0, &normalMat[0][0]);
		mvp_loc = glGetUniformLocation(shader_program, "modelViewProjMat");
		glUniformMatrix4fv(mvp_loc, 1, 0, &modelViewProjMat[0][0]);

		glBindVertexArray(cube_vao);
		glDrawElements(GL_TRIANGLES, cube_vertexCount, GL_UNSIGNED_INT, NULL);
	}
}

void Cleanup()
{
    glDeleteBuffers(1, &cube_vbo);
    glDeleteBuffers(1, &cube_ebo);
    glDeleteVertexArrays(1, &cube_vao);

    glUseProgram(GL_NONE);
    glDeleteProgram(shader_program);
}

void GUI()
{
    ImGui::Begin("Settings");
    {
        // Show some basic stats in the settings window 
        ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
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

    window = glfwCreateWindow(width, height, "Laboratory 2", NULL, NULL);
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