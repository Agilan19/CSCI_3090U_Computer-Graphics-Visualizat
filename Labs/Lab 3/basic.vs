#version 330 core

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec3 vColor;

// We seperated the modelViewProjMat from last week into
// a modelMat and a viewProjMat. This is because we need
// modelMat seperate to compute worldPos

uniform mat4 normalMat;
uniform mat4 modelMat;
uniform mat4 viewProjMat;

out vec3 normal;
out vec3 color;
out vec3 worldPos;

void main()
{
	gl_Position = viewProjMat * modelMat * vec4(vPosition, 1.0);

	// worldPos is a vec3. this is the xyz coordinate of the vertex in the world
	// after we've transformed it with our transformation matrix (aka modelMatrix).
	// so if we moved our object to the right 10 units, this worldPos will have that
	// new location applied to it. vPosition doesn't have that information applied to it.

	worldPos = (modelMat * vec4(vPosition, 1.0)).xyz;


	normal = (normalMat * vec4(vNormal, 1.0)).xyz;

	// The only other addition this week is the color. This is a vertex color, and comes
	// from the .OBJ file we loaded in.
	color = vColor;
}