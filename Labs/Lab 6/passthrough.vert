#version 400
in vec3 vp;
in vec3 vn;
smooth out vec3 normal;

uniform mat4 m;
uniform mat4 v;
uniform mat4 p;

uniform mat4 n; // normal matrix

void main()
{
    gl_Position = p * v * m * vec4(vp, 1.0);

    normal = (n * vec4(vn, 1.0)).xyz;
}