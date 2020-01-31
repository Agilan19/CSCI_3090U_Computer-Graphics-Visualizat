#version 400
out vec4 frag_colour;
in vec3 normal;

uniform float specularPower;
uniform vec3 specularColor;
uniform vec3 diffuseColor;

void main()
{
    vec3 N = normalize(normal);
    vec3 L = normalize(vec3(2, 5, -1));

    // compute diffuse
    float NoL = dot(N, L);
    float diffuse = NoL;

    // compute specular
    vec3 H = normalize(N + L);
    float NoH = dot(N, H);
    float specular = pow(NoH, specularPower);

    // and ambient is hardcoded, to 0.2
    vec3 ambient = vec3(0.1f);

    // Get the final color here
    vec3 finalColor = ambient + diffuse * diffuseColor + specular * specularColor;

    frag_colour = vec4(finalColor, 1.0);
}