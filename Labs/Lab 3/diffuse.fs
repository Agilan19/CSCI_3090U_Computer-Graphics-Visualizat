#version 330 core

uniform vec4 lightPosDir;	// XYZ = pos/dir, W = point or dir?
uniform vec4 lightColor;	// RGB = color, A = light intensity

struct Light
{
	int type;
	vec3 direction;
	vec3 position;
	vec3 color;
	float intensity;
} light;

in vec3 normal;
in vec3 color;
in vec3 worldPos;

out vec4 outColor;

void main()
{
	///////////////////////////////////////// Create the light source ////////////////////////////////
	// this is 0 for a directional light, 1 for a point light
	light.type = int(lightPosDir.w); 
	light.color = lightColor.rgb;
	light.intensity = lightColor.a;

	if (light.type == 0)	// If it's a directional light, set the direction
	{
		light.position = vec3(0);
		light.direction = -normalize(lightPosDir.xyz);
		// We don't have attenuation so we don't need this being so big. let's make
		// it 0 to 1
		light.intensity = light.intensity / 150.0f; 
	}
	else if (light.type == 1)	// If it's a point light, set the position
	{
		light.position = lightPosDir.xyz;
		light.direction = normalize(worldPos - light.position);
	}
	
	///////////////////////////////////////// Use the light source ////////////////////////////////
	vec3 ambient = vec3(0.25f); // Ambient intensity is hardcoded to 0.4 here
	vec3 diffuse = vec3(0.0f);
	vec3 specular = vec3(0.0f);	// Specular is going to stay black for today

	if (light.type == 0)
	{
		float NdotL = dot(normal, -1.0f * light.direction);

		if (NdotL > 0.0f)
		{
			diffuse = NdotL * light.color * light.intensity;
		}
	}
	else if (light.type == 1)
	{
		// Implement a spotlight here
		vec3 forward = -normalize (light.position);

		// Implement a point light here
		float NdotL = dot(normal, -1.0f * light.direction);
		float dist = length(worldPos - light.position);
		float attenuation = 1.0f / (dist * dist);

		// Implement a spotlight here
		if (dot(forward, light.direction) < 0.95f) attenuation = 0.0f;

		if (NdotL > 0.0f)
		{
			diffuse = NdotL * light.color * light.intensity * attenuation;
		}

	}

	// We don't need to change anything down here.
	outColor.rgb = color * (ambient + diffuse + specular);
	outColor.a = 1.0f;

}