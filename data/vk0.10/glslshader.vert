/*
 * Vertex shader used by Cube demo.
 */
#version 140
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
layout(binding = 0) uniform buf {
        mat4 MVP;
        vec4 position[12*3];
        vec4 attr[12*3];
} ubuf;

layout (location = 0) out vec4 texcoord;
void main() 
{
   texcoord = ubuf.attr[gl_VertexID];
   gl_Position = ubuf.MVP * ubuf.position[gl_VertexID];

   // GL->VK conventions
   gl_Position.y = -gl_Position.y;
   gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
}
