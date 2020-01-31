#version 400

in vec3 vp;
in vec3 vn;

smooth out vec3 normal;
out vec3 worldPos;
out vec3 eyePos;

uniform mat4 m;
uniform mat4 v;
uniform mat4 p;

uniform vec3 cameraPosition;

void main()
{
	worldPos = (m * vec4(vp, 1.0f)).xyz;
	eyePos = cameraPosition;
    normal = vn;

    gl_Position = p * v * m * vec4(vp, 1.0f);

}