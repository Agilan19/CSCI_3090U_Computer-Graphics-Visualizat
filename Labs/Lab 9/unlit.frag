#version 330 core

uniform vec4 color;

// Getting all of the properties from the vertex
in VertexData
{
    vec3 normal;
    vec3 eyePos;
    vec3 worldPos;
} inData;

out vec4 outColor;

void main()
{
	outColor.rgb = color.rgb * color.a;
	outColor.a = 1.0f;
}