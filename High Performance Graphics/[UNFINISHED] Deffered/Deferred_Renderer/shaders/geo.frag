#version 450

layout (binding = 6) uniform sampler2D texSampler;

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inColor;
layout (location = 2) in float inDepth;
layout (location = 3) in vec2 inTexCoord;

layout (location = 0) out vec4 outCol;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out float outSpec;
//layout (location = 3) out float outDepth;

void main() 
{
	//outDepth = inDepth;

	outSpec = 1.;

	// Calculate normal in tangent space
	//vec3 N = normalize(inNormal);
	//vec3 T = normalize(inTangent);
	//vec3 B = cross(N, T);
	//mat3 TBN = mat3(T, B, N);
	//vec3 tnorm = TBN * normalize(texture(samplerNormalMap, inUV).xyz * 2.0 - vec3(1.0));
	//outNormal = vec4(tnorm, 1.0);
	outNormal = vec4(inNormal, 1.0);

	outCol = texture(texSampler, inTexCoord);
}