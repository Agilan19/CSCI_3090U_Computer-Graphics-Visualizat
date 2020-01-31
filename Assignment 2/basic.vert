#version 330 core

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vNormal;

// Sending all of the properties to the fragment shader
out VertexData
{
    vec3 normal;
    vec3 eyePos;
    vec3 worldPos;
} outData;

uniform mat4 modelMat;
uniform mat4 viewProjMat;
uniform vec3 cameraPos;

void main()
{
	outData.worldPos    = (modelMat * vec4(vPosition, 1.0)).xyz;
	outData.normal      = vNormal;
    outData.eyePos      = cameraPos;
    
	gl_Position = viewProjMat * modelMat * vec4(vPosition, 1.0);
}