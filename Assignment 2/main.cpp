// Agilan Ampigaipathar (100553054)

//CUBE AND SPHERE are my two new 3D objects, last time, they were rectangular prisms

// Earth is mapped onto the sphere, sun is mapped onto the cube. 

// Press left arrow key to play with lights 

// One or more of the types of light is working properly, spotlight was removed, result did not work, gave compile errors. 

// OpenGL libraries
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/constants.hpp>
#include <SOIL.h>

// GUI Library
#include <imgui.h>
#include <imgui_impl_glfw_gl3.h>

// Standard Libraries
#include <iostream> // Used for std::cout
#include <vector>   // Used for std::vector<vec3>
#include <map>      // Used for std::map

// Custom headers
#include "shaders.h"
#include "mesh.h"

using namespace glm;

/*---------------------------- Variables ----------------------------*/
// GLFW window
GLFWwindow* window;
int width = 1280, height = 720;

// Shader programs
GLuint phongProgram, skyboxProgram, emissiveProgram;

// Variables for uniforms
mat4 projectionMatrix, viewMatrix, modelMatrix[3];
vec3 cameraPosition, cameraTarget, lightPosition;

// Solar system variables
float earthDays = 17.62f;
float moonRotation = 0.0f;
float simulationSpeed = 4.10f;
int viewMode = 3;

bool drawCube = false;
bool isOpen = false;

// Textures
GLuint skyboxTexture;
GLuint diffuseTexture, specularTexture;
GLuint moonTexture, sunTexture;

enum
{
    EARTH = 0,
    SUN = 1,
    MOON = 2,
};

void Initialize()
{
    // Make a simple shader for the sphere we're drawing
    {
        GLuint vs = buildShader(GL_VERTEX_SHADER, ASSETS"simpleLights.vert");
        GLuint fs = buildShader(GL_FRAGMENT_SHADER, ASSETS"simpleLights.frag");
        phongProgram = buildProgram(vs, fs, 0);
        phongProgram = linkProgram(phongProgram);
        dumpProgram(phongProgram, "Simple program for phong lighting");
    }

    // Make a simple shader for the skybox
    {
        GLuint vs = buildShader(GL_VERTEX_SHADER, ASSETS"skybox.vert");
        GLuint fs = buildShader(GL_FRAGMENT_SHADER, ASSETS"skybox.frag");
        skyboxProgram = buildProgram(vs, fs, 0);
        skyboxProgram = linkProgram(skyboxProgram);
        dumpProgram(skyboxProgram, "Simple program for the skybox");
    }

    // Make a simple shader for the sun
    {
        GLuint vs = buildShader(GL_VERTEX_SHADER, ASSETS"emissive.vert");
        GLuint fs = buildShader(GL_FRAGMENT_SHADER, ASSETS"emissive.frag");
        emissiveProgram = buildProgram(vs, fs, 0);
        emissiveProgram = linkProgram(emissiveProgram);
        dumpProgram(emissiveProgram, "Simple program for the sun");
    }

    // Load in all 6 faces of the skybox cube
    skyboxTexture = SOIL_load_OGL_cubemap
    (
        ASSETS"textures/star_sky/stars.png", // posx
        ASSETS"textures/star_sky/stars.png", // negx
        ASSETS"textures/star_sky/stars.png", // posy
        ASSETS"textures/star_sky/stars.png", // negy
        ASSETS"textures/star_sky/stars.png", // posz
        ASSETS"textures/star_sky/stars.png", // negz
        SOIL_LOAD_RGB,      // This means we're expecting it to have RGB channels
        SOIL_CREATE_NEW_ID, // This means we want to create a new texture instead of overwriting one 
        SOIL_FLAG_MIPMAPS   // This means we want it to generate mip-maps.
    );

    diffuseTexture = SOIL_load_OGL_texture
    (
        ASSETS"textures/earthDiffuse.png",
        SOIL_LOAD_AUTO,
        SOIL_CREATE_NEW_ID,
        SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y | SOIL_FLAG_NTSC_SAFE_RGB | SOIL_FLAG_COMPRESS_TO_DXT
    );

    specularTexture = SOIL_load_OGL_texture
    (
        ASSETS"textures/earthSpecular.png",
        SOIL_LOAD_AUTO,
        SOIL_CREATE_NEW_ID,
        SOIL_FLAG_INVERT_Y | SOIL_FLAG_NTSC_SAFE_RGB | SOIL_FLAG_COMPRESS_TO_DXT
    );

    moonTexture = SOIL_load_OGL_texture
    (
        ASSETS"textures/moonTexture.png",
        SOIL_LOAD_AUTO,
        SOIL_CREATE_NEW_ID,
        SOIL_FLAG_INVERT_Y | SOIL_FLAG_NTSC_SAFE_RGB | SOIL_FLAG_COMPRESS_TO_DXT
    );

    sunTexture = SOIL_load_OGL_texture
    (
        ASSETS"textures/sunTexture.png",
        SOIL_LOAD_AUTO,
        SOIL_CREATE_NEW_ID,
        SOIL_FLAG_INVERT_Y | SOIL_FLAG_NTSC_SAFE_RGB | SOIL_FLAG_COMPRESS_TO_DXT
    );

    cameraPosition = vec3(0, 0, -5);
    cameraTarget = vec3(0, 0, 0);
}

void Update(float deltaTime)
{
    mat4 identity, translation, scaling, rotation;
    identity = mat4(1.0f);
    const float pi2 = 2.0f * pi<float>();

    // Add to the rotation, in days.
    earthDays += deltaTime * simulationSpeed;

    float planetRotations = earthDays;

    if (viewMode == 4) planetRotations = 17.62f;

    // Compute orbits and rotations
    float earthOrbit    = (planetRotations / 365.0f) * pi2;
    float earthRotate   = fract(planetRotations) * pi2;
    float moonOrbit     = (planetRotations / 27.322f) * pi2;
    float moonRotate    = -fract(planetRotations / 27.0f) * pi2;

    vec3 earthPosition = vec3(cos(earthOrbit), 0, sin(earthOrbit)) * 30.0f;
    vec3 moonPosition = earthPosition + vec3(cos(moonOrbit), 0, sin(moonOrbit)) * 4.0f;

	translation = translate(identity, earthPosition);
    scaling = scale(identity, vec3(1)); // The earth stays the same size
	//if (glfwGetKey(window, GLFW_KEY_LEFT)) {
		rotation = rotate(identity, earthRotate, vec3(0, 1, 0));
	//}
    //rotation = rotate(identity, earthRotate, vec3(0, 1, 0));
    modelMatrix[EARTH] = translation * rotation * scaling;

    translation = translate(identity, moonPosition);
    scaling = scale(identity, vec3(0.27)); // The moon is 27% the size of earth
    rotation = rotate(identity, moonRotate, vec3(0, 1, 0));
    modelMatrix[MOON] = translation * rotation * scaling;

    scaling = scale(identity, vec3(3)); // Make the sun 10 times larger
    modelMatrix[SUN] = scaling * identity;

    //if (viewMode == 0)
    //{   // Look from in front of the earth, at the earth
        //viewMatrix = inverse(lookAt(earthPosition + normalize(vec3(-3.0f, 1.0f, 3.0f)) * 5.0f, earthPosition, vec3(0, 1, 0)));
    //}
    //else if (viewMode == 1)
    //{   // Look from behind the earth, at the earth and moon
      //  viewMatrix = inverse(lookAt(earthPosition + normalize(vec3(-3.0f, -2.0f, 3.0f)) * -5.0f, (earthPosition + moonPosition) * 0.5f, vec3(0, 1, 0)));
    //}
    //else if (viewMode == 2)
    //{   // Look from the sun, at the earth and moon
      //  viewMatrix = inverse(lookAt(vec3(0.0f), (earthPosition + moonPosition) * 0.5f, vec3(0, 1, 0)));
    //}
    //else if (viewMode == 3)
    //{   // Look from the moon, at the earth
        viewMatrix = inverse(lookAt(moonPosition, (earthPosition), vec3(0, 1, 0)));
    //}
    //else if (viewMode == 4)
    //{   // Static view
      //  viewMatrix = inverse(lookAt(moonPosition, (earthPosition), vec3(0, 1, 0)));
    //}

    std::cout << planetRotations << std::endl;
}

void Render()
{
    //------------------------------------------------------------------------------------------------ Draw Skybox

    {
        // Use the special skybox program
        glUseProgram(skyboxProgram);                                    // <- Use the skybox shader program. This has the vertex and fragment  shader for the skybox

        // Getting uniform locations  
        GLuint sLoc = glGetUniformLocation(skyboxProgram, "skybox");    // <- Get the uniform location for the skybox
        GLuint vLoc = glGetUniformLocation(skyboxProgram, "view");      // <- Get the uniform location for the view matrix
        GLuint pLoc = glGetUniformLocation(skyboxProgram, "proj");      // <- Get the uniform location for the projection matrix

        // Binding skybox texture
        glUniform1i(sLoc, 0);                                           // <- 1) Get the uniform location for the cubemap sampler, and set it to index zero                       
        glActiveTexture(GL_TEXTURE0);                                   // <- 2) Set the active texture to also be index zero, matching above                             
        glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);              // <- 3) Bind the skybox texture. This texture is bound to zero, so it will be sampled              
                                   
        // Passing up view-projection matrix
        glUniformMatrix4fv(vLoc, 1, GL_FALSE,                           // <- Pass through a special version of the view matrix. This has no position information, as
            &inverse(mat4(mat3(viewMatrix)))[0][0]);                    //    the position was removed by downcasting to mat3, then back up to mat4. It's inverted as well
        glUniformMatrix4fv(pLoc, 1, GL_FALSE, &projectionMatrix[0][0]); // <- Pass through the projection matrix here to the vertex shader

        // Drawing the skybox
        Primitive::DrawSkybox();                                        // <- Draw the skybox here. It's an inverted cube around the camera                                     
                          
        // Unbinding the texture and program
        glBindTexture(GL_TEXTURE_CUBE_MAP, GL_NONE);                    // <- Unbind the texture after we've drawn the skybox here                                  
        glUseProgram(GL_NONE);                                          // <- Unbind the shader program after we've used it here                                    
    }

    //------------------------------------------------------------------------------------------------ Draw Models

    {   //----------------------------------------------------------- EARTH ----------------------------------------------------------------------
        // Use the phong program
        glUseProgram(phongProgram);                                         // <- Use the phong lighting shader program

        // Getting uniform locations
        GLuint dtLoc = glGetUniformLocation(phongProgram, "diffuseTex");    // <- Get the uniform location for the diffuse texture
        GLuint stLoc = glGetUniformLocation(phongProgram, "specularTex");   // <- Get the uniform location for the diffuse texture
        GLuint cLoc = glGetUniformLocation(phongProgram, "cameraPos");      // <- Get the uniform location for the projection matrix
        GLuint mLoc = glGetUniformLocation(phongProgram, "model");          // <- Get the uniform location for the model matrix
        GLuint vLoc = glGetUniformLocation(phongProgram, "view");           // <- Get the uniform location for the view matrix
        GLuint pLoc = glGetUniformLocation(phongProgram, "proj");           // <- Get the uniform location for the projection matrix
        GLuint nLoc = glGetUniformLocation(phongProgram, "norm");           // <- Get the uniform location for the normal matrix

        // Binding diffuse texture
        glUniform1i(dtLoc, 0);                                              // <- 1) Get the uniform location for the 2D sampler, and set it to index zero                       
        glActiveTexture(GL_TEXTURE0);                                       // <- 2) Set the active texture to also be index zero, matching above                             
        glBindTexture(GL_TEXTURE_2D, diffuseTexture);                       // <- 3) Bind the diffuse texture (bound to index 0)

        // Binding specular texture
        glUniform1i(stLoc, 1);                                                                     
        glActiveTexture(GL_TEXTURE1);                                                           
        glBindTexture(GL_TEXTURE_2D, specularTexture);                      

        // Passing MVP matrix
        glUniformMatrix4fv(mLoc, 1, GL_FALSE, &modelMatrix[EARTH][0][0]);   // <- Pass through the model matrix here to the vertex shader
        glUniformMatrix4fv(vLoc, 1, GL_FALSE, &inverse(viewMatrix)[0][0]);  // <- Pass through the inverse of the view matrix here to the vertex shader
        glUniformMatrix4fv(pLoc, 1, GL_FALSE, &projectionMatrix[0][0]);     // <- Pass through the projection matrix here to the vertex shader
        glUniformMatrix4fv(nLoc, 1, GL_FALSE,                               // <- Pass through the transpose of the inverse of the model matrix
            &transpose(inverse(modelMatrix[EARTH]))[0][0]);                 //    so that we correctly transform the normals into world space as well

        // Passing up additional information
        glUniform3fv(cLoc, 1, &cameraPosition[0]);                          // <- Pass through the camera location to the shader

        Primitive::DrawSphere();    // Earth

        // Unbinding textures
        glActiveTexture(GL_TEXTURE1);                               // <- Set Texture 1 to be active, and then                
        glBindTexture(GL_TEXTURE_2D, GL_NONE);                      // <- unbind it here
        glActiveTexture(GL_TEXTURE0);                               // <- Set Texture 0 to be active, and then         
        glBindTexture(GL_TEXTURE_2D, GL_NONE);                      // <- unbind it here            

        //----------------------------------------------------------- THE MOON (see above for comments) --------------------------------------------------

        // Binding diffuse texture
        glUniform1i(dtLoc, 0);                                                           
        glActiveTexture(GL_TEXTURE0);                                                 
        glBindTexture(GL_TEXTURE_2D, moonTexture);                          

        // Binding specular texture
        glUniform1i(stLoc, 1);                                                          
        glActiveTexture(GL_TEXTURE1);                                                
        glBindTexture(GL_TEXTURE_2D, moonTexture);                          

        // Passing MVP matrix
        glUniformMatrix4fv(mLoc, 1, GL_FALSE, &modelMatrix[MOON][0][0]);    
        glUniformMatrix4fv(vLoc, 1, GL_FALSE, &inverse(viewMatrix)[0][0]);  
        glUniformMatrix4fv(pLoc, 1, GL_FALSE, &projectionMatrix[0][0]);     
        glUniformMatrix4fv(nLoc, 1, GL_FALSE,                               
            &transpose(inverse(modelMatrix[MOON]))[0][0]);                  

        // Passing up additional information
        glUniform3fv(cLoc, 1, &cameraPosition[0]);                     

        Primitive::DrawSphere();    // Moon

        // Unbinding textures
        glActiveTexture(GL_TEXTURE1);                                         
        glBindTexture(GL_TEXTURE_2D, GL_NONE);                   
        glActiveTexture(GL_TEXTURE0);                                  
        glBindTexture(GL_TEXTURE_2D, GL_NONE);                   

        // unbinding the shader program
        glUseProgram(GL_NONE);

        //----------------------------------------------------------- THE SUN (see above for comments) ----------------------------------------------------

        glUseProgram(emissiveProgram);                                        

        mLoc = glGetUniformLocation(emissiveProgram, "model");
        vLoc = glGetUniformLocation(emissiveProgram, "view");
        pLoc = glGetUniformLocation(emissiveProgram, "proj");
        GLuint etLoc = glGetUniformLocation(emissiveProgram, "emissiveTex");

        // Binding emissive texture
        glUniform1i(etLoc, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sunTexture);

        // Passing MVP matrix
        glUniformMatrix4fv(mLoc, 1, GL_FALSE, &modelMatrix[SUN][0][0]);
        glUniformMatrix4fv(vLoc, 1, GL_FALSE, &inverse(viewMatrix)[0][0]);
        glUniformMatrix4fv(pLoc, 1, GL_FALSE, &projectionMatrix[0][0]);

        //Primitive::DrawSphere();    // Sun

		Primitive::DrawBox();

		// if (drawCube)   Primitive::DrawBox();
		// else            Primitive::DrawSphere();

        // Unbinding textures
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, GL_NONE);

        // unbinding the shader program
        glUseProgram(GL_NONE);
    }
}

void Cleanup()
{
    // Cleanup the shader programs here
    glDeleteProgram(skyboxProgram);
    glDeleteProgram(phongProgram);

    // Cleanup the textures here
    glDeleteTextures(1, &skyboxTexture);
    glDeleteTextures(1, &diffuseTexture);
    glDeleteTextures(1, &specularTexture);
}

void GUI()
{
    ImGui::Begin("Assignment 2", &isOpen);
    {
        ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);

        ImGui::Spacing();
        ImGui::DragFloat("Simulation Speed", &simulationSpeed, 0.01f, 100.0f); simulationSpeed = clamp(simulationSpeed, 0.01f, 100.0f);
		ImGui::Text("Press/Hold the Left Arrow Key", 1);
		//ImGui::RadioButton("View 1", &viewMode, 0); ImGui::SameLine();
        //ImGui::RadioButton("View 2", &viewMode, 1); ImGui::SameLine();
        //ImGui::RadioButton("View 3", &viewMode, 2); ImGui::SameLine();
        //ImGui::RadioButton("View 4", &viewMode, 3);

        //ImGui::RadioButton("Static View", &viewMode, 4);

		// ImGui::Checkbox("Draw Cube", &drawCube);
    }
    ImGui::End();
}

void OnWindowResized(GLFWwindow* win, int w, int h)
{
    // Set the viewport incase the window size changed
    width = w; height = h;
    glViewport(0, 0, width, height);
    float ratio = width / (float)height;
    projectionMatrix = perspective(radians(40.0f), ratio, 0.1f, 1000.0f);
}

int main()
{
    // start GL context and O/S window using the GLFW helper library
    if (!glfwInit()) {
        fprintf(stderr, "ERROR: could not start GLFW3\n");
        return 1;
    }

    window = glfwCreateWindow(width, height, "Assignment 2", NULL, NULL);
    if (!window) {
        fprintf(stderr, "ERROR: could not open window with GLFW3\n");
        glfwTerminate();
        return 1;
    }
    glfwSetWindowSizeCallback(window, OnWindowResized);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0); // essentially turn off vsync

    // start GL3W
    gl3wInit();

    // Resize at least once
    OnWindowResized(window, width, height);

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
    glEnable(GL_CULL_FACE);
    glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"

    Initialize();

    float oldTime = 0.0f, currentTime = 0.0f, deltaTime = 0.0f;
    while (!glfwWindowShouldClose(window))
    {
		if (glfwGetKey(window, GLFW_KEY_LEFT)) {
			do { currentTime = (float)glfwGetTime(); } while (currentTime - oldTime < 1.0f / 120.0f);
		}
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