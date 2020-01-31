/*
 *  Simple fragment shader for Lab 1
 */

#version 330 core

in vec3 normal;

void main()
{
	vec3 color = vec3(1.0f, 0.5f, 1.0f);
	vec3 L = normalize(vec3(0.5f, 1.0f, 1.3f));

	vec3 N = normalize(normal);
	float NoL = dot(N,L);

	vec3 diffuse = color * NoL;

	gl_FragColor.rgb = diffuse;
	gl_FragColor.a = 1.0f;

}