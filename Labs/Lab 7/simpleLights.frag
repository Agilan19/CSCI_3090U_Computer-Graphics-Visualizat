#version 400

out vec4 frag_colour;

in vec3 normal;
in vec3 worldPos;
in vec3 eyePos;

uniform samplerCube skybox;

uniform float specularPower;
uniform float ambientLevel;
uniform vec3 tintColor;

uniform vec3 lightDir;

void main()
{
    vec3 N = normalize(normal);
    float NoL = dot(N, lightDir);

	/*  Skybox sampling can go here. There are 3 steps. 
	 *  1) Compute the view direction
	 *  2) Reflect the view direction on the normal
	 *  3) Sample the texture. For this step, replace the following line of code.
	 */

	 vec3 viewDirection = normalize(worldPos - eyePos);
	 vec3 reflectedVector = reflect(viewDirection, N);
	 //vec3 refractedVector = refract(viewDirection, N, 1.31f);

	vec4 reflectedColor = texture(skybox, reflectedVector); // When you sample the cubemap, set it to this variable
	
	vec3 ambient = vec3(0.0f);
	vec3 diffuse = vec3(0.0f);
	vec3 specular = vec3(0.0f);
	
	// compute ambient
	ambient = vec3(ambientLevel);

    // compute diffuse, if NDotL is larger than 0. Otherwise, it will be black
	if (NoL > 0.0f)
	{
		// We use N dot L directly for diffuse light
		diffuse = vec3(NoL); 
	}

	// compute specular.
	
	// Step 1)
	vec3 reflectedLight = reflect(-lightDir, N);
	// Step 2)
	float cosAngle = max(0.0f, dot(reflectedLight, -viewDirection)); // clamp brings it in 0-1 range
	// Step 3)
	specular = vec3(pow(cosAngle, specularPower)); //* vec3(1.0f, 0.0f, 0.0f); 

	/*  Specular code can go here. 
	 *  Refer to the document for example code, and an explanation. Make sure that variables are normalized when they should be,
	 *  and that you clamp the resulting vector in the 0-1 range. Following the document should do this. Store your results in
	 *  the specular variable, and it will be used below.
	 */

	// And we have the resulting phong lighting
    frag_colour.rgb = reflectedColor.rgb * tintColor * (ambient + diffuse + specular);
	frag_colour.a = 1.0f;
}