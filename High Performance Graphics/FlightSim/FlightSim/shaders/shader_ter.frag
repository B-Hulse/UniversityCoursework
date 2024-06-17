#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec4 l_pos;
    vec4 m_ambient;
    vec4 m_diffuse;
    vec4 m_specular;
    vec4 m_emission;
    float m_shininess;
} ubo;

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragColor;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;

void main() {
    outColor = vec4(fragColor, 1.);

    vec3 N = normalize(fragNormal);
    vec3 L = normalize(ubo.l_pos.xyz);

    float lamb = max(dot(N,L),0.0);
    float spec = 0.0;

    if (lamb > 0.0) {
        vec3 R = reflect(-L,N);
        vec3 V = normalize(-fragPos);

        float specAngle = max(dot(R,V),0.0);
        spec = pow(specAngle, ubo.m_shininess);
    }

    vec4 l_col = vec4(1.,1.,1.,1.);

    outColor = vec4(l_col * ubo.m_ambient +
                    l_col * ubo.m_diffuse * lamb +
                    l_col * ubo.m_specular * spec +
                    ubo.m_emission);
    outColor.w = ubo.m_diffuse.w;

    outColor *= texture(texSampler, fragTexCoord);
}