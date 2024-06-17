#version 450
#extension GL_ARB_separate_shader_objects : enable
//#extension GL_KHR_vulkan_glsl : enable

#define PI 3.1415926538
#define WIDTH 100

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec4 l_pos;
    vec4 m_ambient;
    vec4 m_diffuse;
    vec4 m_specular;
    vec4 m_emission;
    float m_shininess;
    float binWidth;
    int binQuads;

} ubo;

layout(push_constant) uniform PushConstants {
    vec2 start_pos;
} push;

layout(binding = 2) uniform sampler2D hmapSampler;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragColor;
layout(location = 2) out vec2 fragTexCoord;
layout(location = 3) out vec3 fragNormal;

// Get the position of the current vertex
// The current bin is assumed to be a binSplits x binSplits set of rects
vec2 getVertPos(int vi) {
    // Get the index of the quad the vert belongs to
    int quad_i = int(floor(float(vi)/6.f));

    // Get the x,y index into the bin of the quad
    ivec2 quad_xy = ivec2(quad_i % ubo.binQuads, int(floor(float(quad_i)/float(ubo.binQuads))));

    // Width of a quad
    float binQuadW = ubo.binWidth / float(ubo.binQuads);

    // Get the index within the quad that the vert belongs to
    int vert_i = vi - (quad_i*6);

    // Position of the quad that the vert is in
    vec2 quadPos = push.start_pos + (binQuadW * quad_xy);

    if (vert_i == 0)                return quadPos;
    if (vert_i == 1 || vert_i == 4) return quadPos + vec2(binQuadW , 0       );
    if (vert_i == 2 || vert_i == 3) return quadPos + vec2(0        , binQuadW);
    if (vert_i == 5)                return quadPos + vec2(binQuadW , binQuadW);
}

void main() {
    vec2 vert_pos = getVertPos(gl_VertexIndex);

    float h_offset = -5.;
    float h_scale = 200.f; // Height scale
    float s_scale = .001f; // Sampler scale

    float s_offset = 1.f;

    // Find the hight according to the height map
    //vec4 modelPos = vec4(vert_x + ubo.start_pos.x, vert_y + ubo.start_pos.y, 0., 1.0);
    vec4 modelPos = vec4(vert_pos, 0.,1.);
    vec4 hmapVal = texture(hmapSampler, modelPos.xy * s_scale);
    modelPos.z = h_offset + (hmapVal.x * h_scale);
    
    // get the four points around the vector
    vec3 locals[4];
    locals[0] = modelPos.xyz;
    locals[1] = modelPos.xyz;
    locals[2] = modelPos.xyz;
    locals[3] = modelPos.xyz;
    locals[0].x += s_offset;
    locals[1].x -= s_offset;
    locals[2].y += s_offset;
    locals[3].y -= s_offset;
    locals[0].z = h_offset + (texture(hmapSampler, locals[0].xy * s_scale).x * h_scale);
    locals[1].z = h_offset + (texture(hmapSampler, locals[1].xy * s_scale).x * h_scale);
    locals[2].z = h_offset + (texture(hmapSampler, locals[2].xy * s_scale).x * h_scale);
    locals[3].z = h_offset + (texture(hmapSampler, locals[3].xy * s_scale).x * h_scale);

    vec3 norms[4];
    norms[0] = cross(locals[0], locals[2]);
    norms[1] = cross(locals[2], locals[1]);
    norms[2] = cross(locals[1], locals[3]);
    norms[3] = cross(locals[3], locals[0]);

    vec3 avg_norm = normalize((norms[0] + norms[1] + norms[2] + norms[3]) / 4.f);

    vec4 fragViewPos = ubo.view * modelPos;
    fragPos = vec3(fragViewPos)/ fragViewPos.w;
    gl_Position = ubo.proj * fragViewPos;

    fragNormal = vec3( ubo.view * vec4(avg_norm, 0.f));

    float theta = dot(avg_norm, vec3(0,0,1));

    fragTexCoord.x = 1. -theta;
    fragTexCoord.y = 1.f-hmapVal.x;

    fragColor = vec3(0.,0.,0.);
}