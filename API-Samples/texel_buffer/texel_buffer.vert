#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
layout (binding = 0) uniform samplerBuffer texels;
layout (location = 0) out vec4 outColor;
vec2 vertices[3];
float r;
float g;
float b;
void main() {
    r = texelFetch(texels, 0).r;
    g = texelFetch(texels, 1).r;
    b = texelFetch(texels, 2).r;
    outColor = vec4(r, g, b, 1.0);
    vertices[0] = vec2(-1.0, -1.0);
    vertices[1] = vec2( 1.0, -1.0);
    vertices[2] = vec2( 0.0,  1.0);
    gl_Position = vec4(vertices[gl_VertexIndex % 3], 0.0, 1.0);
}