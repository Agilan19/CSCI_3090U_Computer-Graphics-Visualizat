#version 330 core

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// IMPORTANT - We've made the color variable a uniform variable in the fragment shader. This lets us send
// information to the shader from outside of the shader. We will use it so we can assign unique colors.

uniform vec3 uniqueColor;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

in vec3 normal;

void main()
{
	vec3 L = normalize(vec3(0.5f, 1.0f, 1.3f));

	vec3 N = normalize(normal);
	float NoL = dot(N,L);

	vec3 diffuse = uniqueColor * NoL;

	gl_FragColor.rgb = diffuse;
	gl_FragColor.a = 1.0f;

}