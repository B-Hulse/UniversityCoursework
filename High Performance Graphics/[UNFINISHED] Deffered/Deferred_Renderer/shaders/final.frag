#version 450

layout (binding = 1) uniform sampler2D colSampler;
layout (binding = 2) uniform sampler2D normalSampler;
layout (binding = 3) uniform sampler2D specSampler;
layout (binding = 4) uniform sampler2D depthSampler;

//layout (location = 0) in vec2 inTexCoords;

layout (location = 0) out vec4 outColor;

struct Light {
	vec4 position;
	vec3 color;
};

layout (binding = 5) uniform UBO 
{
	Light lights[1];
	vec4 viewPos;
	int showDebugBuffers;
} ubo;

void main() 
{
	/*
	// Get G-Buffer values
	vec4 fragCol = texture(colSampler, inTexCoords);
	vec3 fragNorm = texture(normalSampler, inTexCoords).rgb;
	float fragSpec = texture(specSampler, inTexCoords).r;
	float fragDepth = texture(depthSampler, inTexCoords).r;
	
	switch (ubo.showDebugBuffers) {
		case 1:
			outColor = fragCol;
			break;
		case 2:
			outColor = vec4(fragNorm, 1.0);
			break;
		case 3:
			outColor = vec4(fragSpec,fragSpec,fragSpec, 1.0);
			break;
		case 4:
			outColor = vec4(fragDepth,fragDepth,fragDepth, 1.0);
			break;
	}
	*/
	outColor = vec4(1.,0.,1.,1.);
	
}