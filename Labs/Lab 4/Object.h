/**************************************************
*
*                 Shaders.h
*
*  Utility functions that make constructing loading
*  in OBJ files easier
*
***************************************************/

#include <GL/gl3w.h>
#include <string>

static const int VERTEX_LOC = 0;
static const int NORMAL_LOC = 1;
static const int COLORS_LOC = 2;

struct Model
{
    GLuint vao;
    GLuint vbo; // interleaved. This means vertex, normal, and colors are all in one

    unsigned int vertexCount;
};

Model Load3DModel(std::string basedir, std::string filename);