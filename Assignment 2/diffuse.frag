#version 330 core

#define MAX_LIGHTS 10   // How many lights are possible. Can't be too high

// Everything below is an array. This is how we get multiple lights in forward-rendering.

uniform vec3 lightPos[MAX_LIGHTS];	    // XYZ = pos
uniform vec3 lightDir[MAX_LIGHTS];	    // XYZ = dir (hopefully normalized)
uniform vec4 lightColor[MAX_LIGHTS];    // RGB = color, A = light intensity
uniform float spotAngle[MAX_LIGHTS];    // Cosine of spotlight cone (spotlights only)
uniform int lightType[MAX_LIGHTS];      // 0 = directional. 1 = point. 2 = spotlight.

uniform int numLights;  // Less than or equal to MAX_LIGHTS

// A structure that contains our lighting information
struct Light    
{
	vec3 direction;
	vec3 color;
    float intensity;
	float attenuation;
};

// Getting all of the properties from the vertex
in VertexData
{
    vec3 normal;
    vec3 eyePos;
    vec3 worldPos;
} inData;

out vec4 outColor;

void SetupDirectionalLight(inout Light light, in int currentLight)
{
    // ----------------------- DIRECTION AND DISTANCE --------------------------------------
	light.direction = -normalize(lightDir[currentLight]); // This is reversed, because we need to do a dot product calculation with it
    light.attenuation = 1.0f; // With a directional light, this is always 1 (no falloff ever)
}

void SetupPointLight(inout Light light, in int currentLight)
{
    // ----------------------- DIRECTION AND DISTANCE --------------------------------------
    vec3 lightDirWMag = inData.worldPos - lightPos[currentLight];

	light.direction = -normalize(lightDirWMag); // We need to compute the direction to the fragment, and reverse it (see above)
    float dist = length(lightDirWMag); // We need to also get the length of this vector
        
    // ----------------------- ATTENUATION --------------------------------------
    light.attenuation = 1.0f / (dist * dist);   // Quadratic attenuation (physically based)
}

void SetupSpotlight(inout Light light, in int currentLight)
{
    // ----------------------- DIRECTION AND DISTANCE --------------------------------------
    vec3 lightDirWMag = inData.worldPos - lightPos[currentLight];

	light.direction = -normalize(lightDirWMag); // Same as a point light
    float dist = length(lightDirWMag); // Same as a point light
        
    // ----------------------- ATTENUATION (CONE ANGLE TOO) --------------------------------------
    float angle = dot(-light.direction, normalize(lightDir[currentLight]));
        
    light.attenuation = 1.0f / (dist * dist); // Quadratic Attenuation
    light.attenuation = (angle < spotAngle[currentLight]) ? 0.0f : light.attenuation; // If it's not in the cone, set attenuation to 0

    // Try this line of code out, if you want a smooth spotlight falloff.
    // http://www.povray.org/documentation/images/reference/spotgeom.png
    // This makes good use of the smoothstep function
    //light.attenuation *= smoothstep(spotAngle[currentLight], 1.0f), angle);
}

// This function creates the light source from the uniforms, and returns the light struct
Light CreateLightSource(in int currentLight)
{
    Light light;

    // These two properties are the same for every type of light
	light.color     = lightColor[currentLight].rgb;
    light.intensity = lightColor[currentLight].a;

    switch(lightType[currentLight])
    {
        case 0: // directional
            SetupDirectionalLight(light, currentLight);
            break;
        case 1: // point
            SetupPointLight(light, currentLight);
            break;
        case 2: // spotlight
            SetupSpotlight(light, currentLight);
            break;
    }

    return light;
}

void main()
{
	///////////////////////////////////////// Use the light source ////////////////////////////////
	vec3 ambient = vec3(0.0f);
	vec3 diffuse = vec3(0.0f);  
	vec3 specular = vec3(0.0f);	

    // For each light, we will ADD to the diffuse and specular. This is because lights are additive. See the following picture
    // https://i.pinimg.com/originals/cc/57/a3/cc57a376835eb4452d0c000aade75f12.jpg
    for (int i = 0; i < numLights; i++)
    {
        Light l = CreateLightSource(i); // This creates a light source. The most imporant things are: Color, Direction, Intensity, and Attenuation
        
        // Create the variables we will need for the lighting
	    vec3 R      = normalize(reflect(-l.direction, inData.normal));
        vec3 V      = normalize(inData.worldPos - inData.eyePos); // FYI: Doesn't need to be done per-light
	    float VoR   = max(0.0f, dot(-V, R));
        float NoL   = max(0.0f, dot(inData.normal, l.direction));

        // Add to the diffuse and specular here
        diffuse += l.color * NoL * l.attenuation * l.intensity;
        specular += l.color * pow(VoR, 200.0f) * (NoL > 0.0f ? 1.0f : 0.0f) * l.attenuation * l.intensity;
    }

	// We don't need to change anything down here.
	outColor.rgb = (ambient + diffuse + specular);
	outColor.a = 1.0f;

}