#version 450
#extension GL_KHR_vulkan_glsl:enable
/*
layout (location = 0) out vec2 outTexCoord;

void main() 
{
	if (gl_VertexIndex==1) 
		outTexCoord.x = 2.f;
	 else 
		outTexCoord.x = 0.f;

	if (gl_VertexIndex==2)
		outTexCoord.y = 2.f;
	else
		outTexCoord.y = 0.f;

	gl_Position = vec4(0.f,0.f, 1.0f, 1.0f);
}
*/

vec2 positions[3] = vec2[](
    vec2(0.0, 0.0),
    vec2(0.0, 2.0),
    vec2(2.0, 0.0)
);

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
}