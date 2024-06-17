#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) out vec2 fragTexCoord;

vec3 verts[] = {
	{-1.,-1.,-1.},
	{ 1.,-1.,-1.},
	{ 1., 1.,-1.},
	{-1., 1.,-1.},
	{-1.,-1., 1.},
	{ 1.,-1., 1.},
	{ 1., 1., 1.},
	{-1., 1., 1.}
};

int v_indices[] = {
    0, 1, 3, 3, 1, 2,
    1, 5, 2, 2, 5, 6,
    5, 4, 6, 6, 4, 7,
    4, 0, 7, 7, 0, 3,
    3, 2, 7, 7, 2, 6,
    4, 5, 0, 0, 5, 1
};

vec2 texCoords[] = {
    { 0.25 , 0     }, //  0 = Top TL
    { 0.5 , 0      }, //  1 = Top TR
    { 0    , 1./3. }, //  2 = Side 1 TL
    { 0.25 , 1./3. }, //  3 = Side 2 TL
    { 0.5  , 1./3. }, //  4 = Side 3 TL
    { 0.75 , 1./3. }, //  5 = Side 4 TL
    { 1    , 1./3. }, //  6 = Side 4 TR
    { 0    , 2./3. }, //  7 = Side 1 BL
    { 0.75 , 2./3. }, //  8 = Side 4 BL
    { 1    , 2./3. }, //  9 = Side 4 BR
    { 0.25 , 2./3. }, // 10 = Bottom TL
    { 0.5  , 2./3. }, // 11 = Bottom TR
    { 0.25 , 1.    }, // 12 = Bottom BL
    { 0.5  , 1.    }  // 13 = Bottom BR
};

int f_indices[] = {
    13, 11, 12, 12, 11, 10, // -z

    11, 4, 10, 10, 4, 3, // +x 

    4, 1, 3, 3, 1, 0, // +z 
    5, 8, 6, 6, 8, 9, // -x 
    7, 10, 2, 2, 10, 3, // +y 
    5, 4, 8, 8, 4, 11  // -y 
};

/*
int f_indices[] = {
    20, 21, 23, 23, 21, 22, // -z
    4, 5, 7, 7, 5, 6, // +x
    16, 17, 19, 19, 17, 18, // +z
    12, 13, 15, 15, 13, 14, // -x
    0, 1, 3, 3, 1, 2, // +y
    8, 9, 11, 11, 9, 10  // -y
};

vec2 texCoords[] = {
    23faceOffsets[5] + vec2(0   , 1./3.),
};
*/



void main() {
    vec4 fragPos = ubo.view * vec4(verts[v_indices[gl_VertexIndex]] * 1100.f,1.0);
    gl_Position = ubo.proj * (fragPos);
    fragTexCoord = texCoords[f_indices[gl_VertexIndex]];
}