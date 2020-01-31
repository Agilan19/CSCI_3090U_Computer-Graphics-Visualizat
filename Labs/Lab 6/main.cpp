
#include <GL\gl3w.h>
#include <GLFW/glfw3.h> // GLFW helper library
#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>

// IMGUI
#include <imgui.h>
#include <imgui_impl_glfw_gl3.h>

#include <fstream>  // Used for 'ifstream'
#include <string>   // Used for 'getline'
#include <iostream> // Used for 'cout'
#include <stdio.h>  // Used for 'printf'
#include <vector>   // Used for 'vector<vec3>'

#include <unordered_map> // Used for fast smooth normals

using namespace glm;

extern "C" {
    #include "readply.h" // For reading in the bunny
}

/*---------------------------- Variables ----------------------------*/
// GLFW window
GLFWwindow* window;

// Uniform locations for matrices
GLuint m_loc, v_loc, p_loc, n_loc, mvp_loc;
// uniform locations for colors
GLuint spec_loc, diff_loc, line_loc;
// uniform location for floats
GLuint pow_loc, tension_loc, divisions_loc, width_loc;

// Shader programs
GLuint bunny_program, bezier_program;

// Vertex Array Objects
GLuint bunny_vao, bunny_vertexCount;
GLuint bezier_vao, bezier_vertexCount;

// model, view, projection, normal matrices
mat4 m, v, p, n, mvp;

// diffuse and specular color
vec3 diffCol, specCol;
vec3 lineCol;

float specPower; // <-- For the bunny
float tension, width; int divisions; // <-- For the spline

// Possible camera locations
vec3 cameraPositions[4] = {
    vec3(0.0f, 0.6f, 1.0f),
    vec3(1.0f, 0.6f, 0.0f),
    vec3(1.0f, 1.6f, 1.0f),
    vec3(-1.0f, -0.6f, 1.0f)
};
vec3 oldPosition = cameraPositions[0]; // Used in the interpolation function
vec3 newPosition = cameraPositions[0]; // Used in the interpolation function
vec3 camPosition; int cam = 0; // Camera position, and cam is used for the GUI buttons
float interpolationValue = 1.0f; // This is 0 to 1, used for the mix function

// ImGUI variables
bool isOpen = false; int part = 0;

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
    // Initializing part 1
    {
        std::string line;

        std::string vertex_shader_contents;
        std::ifstream vstream(ASSETS"passthrough.vert", std::ios::in);
        while (std::getline(vstream, line))
            vertex_shader_contents.append(line).push_back('\n');
        char const * vertex_shader = vertex_shader_contents.c_str();

        std::string fragment_shader_contents;
        std::ifstream fstream(ASSETS"passthrough.frag", std::ios::in);
        while (std::getline(fstream, line))
            fragment_shader_contents.append(line).push_back('\n');
        char const * fragment_shader = fragment_shader_contents.c_str();

        GLuint vs = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vs, 1, &vertex_shader, NULL);
        glCompileShader(vs);
        GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fs, 1, &fragment_shader, NULL);
        glCompileShader(fs);

        CheckShader(vs);
        CheckShader(fs);

        bunny_program = glCreateProgram();
        glAttachShader(bunny_program, fs);
        glAttachShader(bunny_program, vs);
        glLinkProgram(bunny_program);

        // Read in the bunny model here, and then set it up
        ply_model* bunny = readply(ASSETS"bunny.ply");
        bunny_vertexCount = bunny->nface * 3; // 3 vertices per triangle

                                            // this is where we will keep the points and normals
        std::vector<vec3> points;
        std::vector<vec3> normals;

        // this is used for a fast computation of smooth normals
        std::unordered_map<int, std::vector<int>> normalIndices;

        // Here we translate the ply_model struct into a useful list of vertices
        for (int i = 0, j = 0; i < bunny->nface; i++, j += 3)
        {
            ply_face f = bunny->faces[i];

            ply_vertex a = bunny->vertices[f.vertices[0]];
            ply_vertex b = bunny->vertices[f.vertices[1]];
            ply_vertex c = bunny->vertices[f.vertices[2]];

            // using glm math instead of ply_vertex
            vec3 vert1 = vec3(a.x, a.y, a.z);
            vec3 vert2 = vec3(b.x, b.y, b.z);
            vec3 vert3 = vec3(c.x, c.y, c.z);

            // computing the face normals, these are shared
            vec3 normal;
            normal = cross(
                normalize(vert3 - vert1),
                normalize(vert2 - vert1)
            );

            // add the normal indices into the array. These will
            // either be displayed, or just used for the smooth
            // normal computation
            normalIndices[f.vertices[0]].push_back(j + 0);
            normalIndices[f.vertices[1]].push_back(j + 1);
            normalIndices[f.vertices[2]].push_back(j + 2);

            // adding vertices
            points.push_back(vert1);
            points.push_back(vert2);
            points.push_back(vert3);

            // adding normals
            normals.push_back(normal);
            normals.push_back(normal);
            normals.push_back(normal);
        }

        // copy the list over so we can easily allocate the space
        std::vector<vec3> smoothNormals = normals;
        for (auto itr = normalIndices.begin(); itr != normalIndices.end(); itr++)
        {
            // This will compute the 'average' of the shared normals, resulting in a smooth
            // effect on the bunny
            vec3 smoothNormal = vec3(0);
            for (int i = 0; i < (*itr).second.size(); i++) // for each new normal
            {
                smoothNormal += normals[(*itr).second[i]]; // add the normal shared by the vertex
            }
            smoothNormal /= (float)(*itr).second.size(); // average it out

                                                         // This populates our smoothed normal list with the new normals
            for (int i = 0; i < (*itr).second.size(); i++) // add them to their respective indices
            {
                smoothNormals[(*itr).second[i]] = smoothNormal;
            }
        }

        // This interleaved buffer means that the normals and vertices will occupy
        // the same space in memory. It is useful because we can keep the same
        // attribute locations and stride
        vec3* interleaved_buffer = new vec3[bunny_vertexCount * 2];

        for (int i = 0, j = 0; i < bunny_vertexCount * 2; i += 2, j++)
        {
            interleaved_buffer[i + 0] = points[j];
            //interleaved_buffer[i + 1] = normals[j];
            interleaved_buffer[i + 1] = smoothNormals[j];
        }

        GLuint vbo = 0;
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * bunny_vertexCount * 2, &interleaved_buffer[0], GL_STATIC_DRAW);

        bunny_vao = 0;
        glGenVertexArrays(1, &bunny_vao);
        glBindVertexArray(bunny_vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        GLuint vpos_location = glGetAttribLocation(bunny_program, "vp");
        GLuint vnorm_location = glGetAttribLocation(bunny_program, "vn");

        glEnableVertexAttribArray(vpos_location);
        glVertexAttribPointer(vpos_location, 3, GL_FLOAT, GL_FALSE, sizeof(vec3) * 2, (void*)0);

        glEnableVertexAttribArray(vnorm_location);
        glVertexAttribPointer(vnorm_location, 3, GL_FLOAT, GL_FALSE, sizeof(vec3) * 2, (void*)sizeof(vec3));

        m_loc = glGetUniformLocation(bunny_program, "m");
        v_loc = glGetUniformLocation(bunny_program, "v");
        p_loc = glGetUniformLocation(bunny_program, "p");
        n_loc = glGetUniformLocation(bunny_program, "n");

        spec_loc = glGetUniformLocation(bunny_program, "specularColor");
        diff_loc = glGetUniformLocation(bunny_program, "diffuseColor");
        pow_loc = glGetUniformLocation(bunny_program, "specularPower");

        specCol = vec3(1.0f);
        diffCol = vec3(0.85f, 0.078f, 0.23f);

        specPower = 2.0f;
    }

    // Initializing part 2
    {
        std::string line;

        std::string vertex_shader_contents;
        std::ifstream vstream(ASSETS"line.vert", std::ios::in);
        while (std::getline(vstream, line))
            vertex_shader_contents.append(line).push_back('\n');
        char const * vertex_shader = vertex_shader_contents.c_str();

        std::string geometry_shader_contents;
        std::ifstream gstream(ASSETS"line.geom", std::ios::in);
        while (std::getline(gstream, line))
            geometry_shader_contents.append(line).push_back('\n');
        char const * geometry_shader = geometry_shader_contents.c_str();

        std::string fragment_shader_contents;
        std::ifstream fstream(ASSETS"line.frag", std::ios::in);
        while (std::getline(fstream, line))
            fragment_shader_contents.append(line).push_back('\n');
        char const * fragment_shader = fragment_shader_contents.c_str();

        GLuint vs = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vs, 1, &vertex_shader, NULL);
        glCompileShader(vs);
        GLuint gs = glCreateShader(GL_GEOMETRY_SHADER);
        glShaderSource(gs, 1, &geometry_shader, NULL);
        glCompileShader(gs);
        GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fs, 1, &fragment_shader, NULL);
        glCompileShader(fs);

        CheckShader(vs);
        CheckShader(gs);
        CheckShader(fs);

        bezier_program = glCreateProgram();
        glAttachShader(bezier_program, fs);
        glAttachShader(bezier_program, gs);
        glAttachShader(bezier_program, vs);
        glLinkProgram(bezier_program);

        std::vector<vec3> points;

        {   // Read file
            FILE* pFile;
            long lSize;
            char* buffer;
            size_t result;

            pFile = fopen(ASSETS"points.txt", "r");
            if (pFile == NULL)
            {
                printf("ERR: File error"); exit(1);
            }

            // read the first word of the line
            int res = fscanf(pFile, "%i\n", &bezier_vertexCount);

            for (int i = 0; i < bezier_vertexCount; i++)
            {
                vec3 vertex;
                fscanf(pFile, "%f %f %f\n", &vertex.x, &vertex.z, &vertex.y);
                points.push_back(vertex);
            }
        }

        GLuint vbo = 0;
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * bezier_vertexCount, &points[0][0], GL_STATIC_DRAW);

        bezier_vao = 0;
        glGenVertexArrays(1, &bezier_vao);
        glBindVertexArray(bezier_vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        GLuint vpos_location = glGetAttribLocation(bezier_program, "vp");

        glEnableVertexAttribArray(vpos_location);
        glVertexAttribPointer(vpos_location, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void*)0);

        mvp_loc = glGetUniformLocation(bezier_program, "mvp");
        line_loc = glGetUniformLocation(bezier_program, "lineColor");
        tension_loc = glGetUniformLocation(bezier_program, "tension");
        divisions_loc = glGetUniformLocation(bezier_program, "divisions");
        width_loc = glGetUniformLocation(bezier_program, "width");

        lineCol = vec3(0.0f, 0.0f, 0.0f);
        tension = 0.5f;
        divisions = 30;
        width = 0.006f;
    }
}

void Update(float deltaTime)
{
    // Set the viewport incase the window size changed
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
    float ratio = width / (float)height;

    if (part == 0)
    {
        float duration = 2.0f; // This means we want it to take 5 seconds to get from A to B
        interpolationValue += deltaTime * (1.0f / duration); // This increases linearly
        interpolationValue = glm::clamp(interpolationValue, 0.0f, 1.0f);

        // Compute the camera position here
		float smoothInterpolation = glm::smoothstep(0.0f, 1.0f, interpolationValue);
        camPosition = glm::mix(oldPosition, newPosition, smoothInterpolation);

        // compute the matrices and colors
        m = scale(mat4(1.0f), vec3(5.0f)); // scale of 5 for the bunny
        v = lookAt(camPosition, vec3(0.0f, 0.5f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
        p = perspective(1.39626f, ratio, 0.01f, 10.0f); // 80 deg fov
        n = transpose(inverse(v * m));
    }
    else
    {
        mvp = ortho(-ratio, ratio, -1.0f, 1.0f, -10.0f, 10.0f);
        mvp = scale(mvp, vec3(0.75f));
    }
}

void Render()
{
    if (part == 0)
    {
        // send up the uniforms
        glUseProgram(bunny_program);
        glBindVertexArray(bunny_vao);

        glUniformMatrix4fv(m_loc, 1, GL_FALSE, &m[0][0]);
        glUniformMatrix4fv(v_loc, 1, GL_FALSE, &v[0][0]);
        glUniformMatrix4fv(p_loc, 1, GL_FALSE, &p[0][0]);
        glUniformMatrix4fv(n_loc, 1, GL_FALSE, &n[0][0]);

        glUniform3fv(spec_loc, 1, &specCol[0]);
        glUniform3fv(diff_loc, 1, &diffCol[0]);

        glUniform1fv(pow_loc, 1, &specPower);

        // draw points from the currently bound VAO with current in-use shader
        glDrawArrays(GL_TRIANGLES, 0, bunny_vertexCount);
    }
    else
    {
        // send up the uniforms
        glUseProgram(bezier_program);
        glBindVertexArray(bezier_vao);

        glUniformMatrix4fv(mvp_loc, 1, GL_FALSE, &mvp[0][0]);
        glUniform3fv(line_loc, 1, &lineCol[0]);
        glUniform1fv(tension_loc, 1, &tension);
        glUniform1iv(divisions_loc, 1, &divisions);
        glUniform1fv(width_loc, 1, &width);

        // draw points from the currently bound VAO with current in-use shader
        glDrawArrays(GL_LINE_STRIP_ADJACENCY, 0, bezier_vertexCount);
    }
}

void Cleanup()
{
    glDeleteProgram(bunny_program);
    glDeleteProgram(bezier_program);
}

void GUI()
{
    // 1. Show a simple window, with the title settings
    ImGui::Begin("Settings", &isOpen);
    {
        ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

        ImGui::RadioButton("Part 1", &part, 0); ImGui::SameLine();
        ImGui::RadioButton("Part 2", &part, 1);

        if (part == 0) // First part is the bunny, expose those variables
        {
            ImGui::ColorEdit3("Diffuse", &diffCol[0]);
            ImGui::ColorEdit3("Specular", &specCol[0]);
            ImGui::SliderFloat("Specular Power", &specPower, 2.0f, 20.0f, "%.f");

            int oldCam = cam;
            if (interpolationValue == 1.0f)
            {
                ImGui::RadioButton("C1", &cam, 0); ImGui::SameLine();
                ImGui::RadioButton("C2", &cam, 1); ImGui::SameLine();
                ImGui::RadioButton("C3", &cam, 2); ImGui::SameLine();
                ImGui::RadioButton("C4", &cam, 3);
            }
            if (cam != oldCam) // This means we changed something
            {
                oldPosition = newPosition;
                newPosition = cameraPositions[cam];
                interpolationValue = 0.0f; // Restart the interpolation
            }
        }
        else // Second part is the line
        {
            ImGui::ColorEdit3("Line Color", &lineCol[0]);
            ImGui::SliderFloat("Line width", &width, 0.001f, 0.01f, "%.3f");
            ImGui::SliderFloat("Tension", &tension, 0.0f, 1.0f, "%.2f");
            ImGui::SliderInt("Divisions", &divisions, 1, 30);
        }
    }
    ImGui::End();
}

int main()
{
    // start GL context and O/S window using the GLFW helper library
    if (!glfwInit()) {
        fprintf(stderr, "ERROR: could not start GLFW3\n");
        return 1;
    }

    window = glfwCreateWindow(1280, 720, "Laboratory 6", NULL, NULL);
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
    glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"

    Initialize();

    float oldTime = 0.0f, currentTime = 0.0f, deltaTime = 0.0f;
    while (!glfwWindowShouldClose(window))
    {
        do { currentTime = (float)glfwGetTime(); } while (currentTime - oldTime < 1.0f / 120.0f);

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
