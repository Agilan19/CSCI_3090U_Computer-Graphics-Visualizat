#version 330 core

uniform mat4 normalMat;
uniform mat4 modelViewProjMat;

in vec3 vPosition;
in vec3 vNormal;

out vec3 normal;

void main()
{
	gl_Position = modelViewProjMat * vec4(vPosition, 1.0);
	normal = (normalMat * vec4(vNormal, 1.0)).xyz;
}