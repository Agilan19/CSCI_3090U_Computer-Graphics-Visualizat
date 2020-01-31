#version 400
out vec4 frag_colour;

uniform vec3 lineColor;

void main()
{
    frag_colour = vec4(lineColor, 1.0);
}