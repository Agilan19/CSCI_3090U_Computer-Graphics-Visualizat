#include "Object.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <GLM/glm.hpp>
#include <iostream>

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
        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, (basedir + filename).c_str(), basedir.c_str());

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
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * interleavedVBO.size(), &interleavedVBO[0], GL_STATIC_DRAW);

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
    m.vertexCount = (GLuint)vertices.size();

    return m;
}