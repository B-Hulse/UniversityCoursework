#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoord;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 model;
	mat4 view;
} ubo;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outColor;
layout (location = 2) out float outDepth;
layout (location = 3) out vec2 outTexCoord;

void main() 
{
	gl_Position = ubo.projection * ubo.view * ubo.model * vec4(inPosition, 1.0f);
	
	// Vertex position in world space
	outDepth = gl_Position.z;
	
	// Normal in world space
	outNormal = vec3( ubo.view * ubo.model * vec4(inNormal, 0.f));
	
	// Currently just vertex color
	outColor = inColor;

	outTexCoord = inTexCoord;
}