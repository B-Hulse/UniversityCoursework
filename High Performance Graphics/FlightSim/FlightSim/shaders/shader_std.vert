#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragColor;
layout(location = 2) out vec2 fragTexCoord;
layout(location = 3) out vec3 fragNormal;

void main() {
    vec4 fragViewPos = ubo.view * ubo.model * vec4(inPosition,1.0);
    fragPos = vec3(fragViewPos)/ fragViewPos.w;
    gl_Position = ubo.proj * fragViewPos;

    fragColor = inColor;
    fragNormal = vec3( ubo.view * ubo.model * vec4(inNormal, 0.f));

    fragTexCoord = inTexCoord;
}   