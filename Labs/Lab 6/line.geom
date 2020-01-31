#version 400

//Input type
layout (lines_adjacency) in; // 4 points
//output type
layout (triangle_strip, max_vertices = 120) out;

uniform float width;
uniform float tension;
uniform int divisions;

uniform mat4 mvp;

vec4 GetCrossDir(vec3 a, vec3 b)
{
	vec3 dir = normalize(b - a);
	vec3 tangent = cross(dir, vec3(0,0,1));
	return vec4(tangent * width, 0.0);
}

vec3 GetTangent(vec3 p0, vec3 p1, float c)
{
	return (1.0f - c) * ((p0 - p1) / 2.0f);
}

vec3 GetPoint(vec3 P1, vec3 P2, vec3 T1, vec3 T2, float s)
{
	float s2 = s * s;
	float s3 =  s2 * s;

	float h1 =  2 * s3 - 3 * s2 + 1;	// calculate basis function 1
	float h2 = -2 * s3 + 3 * s2;		// calculate basis function 2
	float h3 =   s3 - 2 *s2 + s;		// calculate basis function 3
	float h4 =   s3 -  s2;				// calculate basis function 4
	vec3 p = h1*P1 +					// multiply and sum all funtions
			   h2*P2 +					// together to build the interpolated
			   h3*T1 +					// point along the curve.
			   h4*T2;
	return p;
}

void main()
{
	vec3 pn1 = gl_in[0].gl_Position.xyz;
	vec3 p0  = gl_in[1].gl_Position.xyz;
	vec3 p1  = gl_in[2].gl_Position.xyz;
	vec3 p2  = gl_in[3].gl_Position.xyz;

	vec3 m0 = GetTangent(p1, pn1, tension);
	vec3 m1 = GetTangent(p2, p0, tension);

	for (int i = 0; i < divisions; i++)
	{
		vec4 a = vec4(GetPoint(p0, p1, m0, m1, float(i + 0) / float(divisions)), 1.0f);
		vec4 b = vec4(GetPoint(p0, p1, m0, m1, float(i + 1) / float(divisions)), 1.0f);
		
		vec4 tangent = GetCrossDir(a.xyz, b.xyz);

		gl_Position = mvp * (a - tangent);
		EmitVertex();
	
		gl_Position = mvp * (a + tangent);
		EmitVertex();
    
		gl_Position = mvp * (b - tangent);
		EmitVertex();
	
		gl_Position = mvp * (b + tangent);
		EmitVertex();
	
		EndPrimitive();
	}
}