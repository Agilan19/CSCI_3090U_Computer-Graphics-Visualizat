#version 330 core

layout(location = 0) in vec3 vPosition;

out vec2 fragCoord;

void main()
{
	gl_Position = vec4(vPosition, 1.0);
	fragCoord = vPosition.xy * 0.5f; // We want this to be -0.5 to 0.5 not -1 to 1
}