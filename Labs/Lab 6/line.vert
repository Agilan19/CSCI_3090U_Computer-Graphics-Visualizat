#version 400
in vec3 vp;

uniform mat4 mvp;

void main()
{
    gl_Position = vec4(vp, 1.0);
}