#include <GL/gl3w.h>
#include <GLFW/glfw3.h> // GLFW helper library
#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <iostream>

// IMGUI
#include <imgui.h>
#include <imgui_impl_glfw_gl3.h>

#include "Shaders.h"

#define PI 3.141592
inline float DEG2RAD(float deg) { return (PI * deg / 180.0f); }
inline float RAD2DEG(float rad) { return (rad * (180.0 / PI)); }

static const int VERTEX_LOC = 0;
static const int NORMAL_LOC = 1;
static const int COLORS_LOC = 2;

/*---------------------------- Variables ----------------------------*/
// GLFW window
GLFWwindow* window;
int width = 1280;
int height = 720;

GLuint shader_program;
glm::mat4 viewMatrix;
glm::mat4 projectionMatrix;

glm::vec4 lightPos = glm::vec4(1.0f); 
glm::vec4 lightCol = glm::vec4(1.0f, 1.0f, 1.0f, 100.0f);
bool isPointLight = false;

struct Model
{
    GLuint vao;
    GLuint vbo; // interleaved. This means vertex, normal, and colors are all in one

    unsigned int vertexCount;
};

// 3D models
std::vector<Model> trees;
Model ground;

/*---------------------------- Functions ----------------------------*/
Model Load3DModel(std::string basedir, std::string filename)
{
    Model m;

    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec3> colors;

    {   // We're going to use TinyObjLoader to load in a wavefront OBJ file
        using namespace std;
        using namespace glm;

        // TinyObjLoader: http://syoyo.github.io/tinyobjloader/
        tinyobj::attrib_t attrib;
        vector< tinyobj::shape_t> shapes;
        vector< tinyobj::material_t> materials;

        string err;
        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, (basedir+filename).c_str(), basedir.c_str());

        if (!err.empty()) {
            std::cerr << err << std::endl;
        }

        // Loop over shapes
        for (size_t s = 0; s < shapes.size(); s++)
        {
            // Loop over faces(polygon)
            size_t index_offset = 0;
            for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++)
            {
                int fv = shapes[s].mesh.num_face_vertices[f];

                // per-face material
                tinyobj::material_t m = materials[shapes[s].mesh.material_ids[f]];

                // Loop over vertices in the face.
                for (size_t v = 0; v < fv; v++) {
                    // access to vertex
                    tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

                    vertices.push_back(vec3(
                        attrib.vertices[3 * idx.vertex_index + 0],  // Vertex X
                        attrib.vertices[3 * idx.vertex_index + 1],  // Vertex Y
                        attrib.vertices[3 * idx.vertex_index + 2]   // Vertex Z
                    ));

                    normals.push_back(vec3(
                        attrib.normals[3 * idx.normal_index + 0],   // Normal X
                        attrib.normals[3 * idx.normal_index + 1],   // Normal Y
                        attrib.normals[3 * idx.normal_index + 2]    // Normal Z
                    ));

                    colors.push_back(vec3(
                        m.diffuse[0],   // Color R
                        m.diffuse[1],   // Color G
                        m.diffuse[2]    // Color B
                    ));

                }
                index_offset += fv;
            }
        }
    }

    // 9 because vec3 has 3 parts, and there are 3 vec3s. 3x3=9
    std::vector<float> interleavedVBO(9 * vertices.size());
    // Create an interleaved VBO. This is layout out the following way
    /*
        vec3_vertices, vec3_normals, vec3_colors, vec3_vertices, vec3_normals, vec3_colors, etc...
    */
    for (int i = 0; i < vertices.size(); i++)
    {
        interleavedVBO[i * 9 + 0] = vertices[i].x;
        interleavedVBO[i * 9 + 1] = vertices[i].y;
        interleavedVBO[i * 9 + 2] = vertices[i].z;
        interleavedVBO[i * 9 + 3] = normals[i].x;
        interleavedVBO[i * 9 + 4] = normals[i].y;
        interleavedVBO[i * 9 + 5] = normals[i].z;
        interleavedVBO[i * 9 + 6] = colors[i].x;
        interleavedVBO[i * 9 + 7] = colors[i].y;
        interleavedVBO[i * 9 + 8] = colors[i].z;
    }

    glGenVertexArrays(1, &m.vao);
    glBindVertexArray(m.vao);

    glGenBuffers(1, &m.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m.vbo);

    // glBufferData(......) // implement it here. use the vector container 'interleavedVBO' from above
	glBufferData(
		GL_ARRAY_BUFFER,
		interleavedVBO.size() * sizeof(64),
		&interleavedVBO[0],
		GL_STATIC_DRAW
	);

    // Vertex info
    glVertexAttribPointer(VERTEX_LOC, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3) * 3, (void*)0);
    glEnableVertexAttribArray(VERTEX_LOC);
    // Normal info
    glVertexAttribPointer(NORMAL_LOC, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3) * 3, (void*)sizeof(glm::vec3));
    glEnableVertexAttribArray(NORMAL_LOC);
    // Color info
    glVertexAttribPointer(COLORS_LOC, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3) * 3, (void*)(sizeof(glm::vec3) * 2));
    glEnableVertexAttribArray(COLORS_LOC);

    // Uncomment the line below when you've fixed the code above
    //m.vertexCount = 0; // vertices.size();
	m.vertexCount = vertices.size();

    return m;
}

void Initialize()
{
    // Create a shader for the lab
    GLuint vs = buildShader(GL_VERTEX_SHADER, ASSETS"basic.vs");
    GLuint fs = buildShader(GL_FRAGMENT_SHADER, ASSETS"diffuse.fs");
    shader_program = buildProgram(vs, fs, 0);

    // We need to bind the locations before linking the program
    glBindAttribLocation(shader_program, VERTEX_LOC, "vPosition");
    glBindAttribLocation(shader_program, NORMAL_LOC, "vNormal");
    glBindAttribLocation(shader_program, COLORS_LOC, "vColor");

    // Link and dump the shader errors etc
    linkProgram(shader_program);
    dumpProgram(shader_program, "Diffuse Lighting shader program");

    // Load in the trees
    trees.push_back(Load3DModel(ASSETS"Models/", "treeDecorated.obj"));
    trees.push_back(Load3DModel(ASSETS"Models/", "treePine.obj"));
    trees.push_back(Load3DModel(ASSETS"Models/", "snowmanFancy.obj"));
    trees.push_back(Load3DModel(ASSETS"Models/", "treePineSnowed.obj"));
    trees.push_back(Load3DModel(ASSETS"Models/", "treePineSnowRound.obj"));

    ground = Load3DModel(ASSETS"Models/", "snowPatch.obj");
}

void Update()
{
    lightPos = glm::vec4(sin(glfwGetTime()) * 13.0f, 3.0f, cos(glfwGetTime()) * 13.0f, isPointLight ? 1.0f : 0.0f);
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

float fov = 50.0f; float nearClip = 0.01f;
void Render()
{
    glUseProgram(shader_program);


	viewMatrix = glm::rotate(viewMatrix, DEG2RAD(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	viewMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(sin(glfwGetTime())*5.0f, 2.0f, 10.0f));
	

	viewMatrix = glm::inverse(viewMatrix);

    float ratio = width / (float)height;
    projectionMatrix = glm::perspective(DEG2RAD(fov), 3.0f, nearClip, 100.0f);

    // Set view projection matrix
    GLuint vp_loc = glGetUniformLocation(shader_program, "viewProjMat");
    glUniformMatrix4fv(vp_loc, 1, 0, &(projectionMatrix * viewMatrix)[0][0]);

    // Set lighting information
    GLuint light_loc = glGetUniformLocation(shader_program, "lightPosDir");
    GLuint color_loc = glGetUniformLocation(shader_program, "lightColor"); // RGB is color, A is intensity
    glUniform4fv(light_loc, 1, &lightPos[0]);
    glUniform4fv(color_loc, 1, &lightCol[0]);

    for (int i = 0; i < trees.size(); i++)
    {
        glm::mat4 modelMat = glm::mat4(1.0f);
        modelMat = glm::translate(modelMat, glm::vec3(i * 3 - 6, 0, 0));
        modelMat = glm::scale(modelMat, glm::vec3(0.2f));
        modelMat = glm::rotate(modelMat, DEG2RAD(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        RenderModel(modelMat, trees[i]);
    }

    {   // Draw the Ground
        glm::mat4 modelMat = glm::mat4(1.0f);
        modelMat = glm::translate(modelMat, glm::vec3(10.0f, -0.5f, -10.0f));
        modelMat = glm::scale(modelMat, glm::vec3(0.6f));
        RenderModel(modelMat, ground);
    }
}

void Cleanup()
{
    // cleanup all trees
    for (auto itr = trees.begin(); itr != trees.end(); itr++)
    {
        glDeleteBuffers(1, &(*itr).vbo);
        glDeleteVertexArrays(1, &(*itr).vao);
    }

    // cleanup the snowman
    glDeleteBuffers(1, &ground.vbo);
    glDeleteVertexArrays(1, &ground.vao);

    glUseProgram(GL_NONE);
    glDeleteProgram(shader_program);
}

void GUI()
{
    ImGui::Begin("Settings");
    {
        // Show some basic stats in the settings window 
        ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		ImGui::SliderFloat("Field of view", &fov, 1.0f, 180.0f);
		ImGui::SliderFloat("Near Clip", &nearClip, 0.01f, 20.0f);
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

    window = glfwCreateWindow(width, height, "Laboratory 3", NULL, NULL);
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