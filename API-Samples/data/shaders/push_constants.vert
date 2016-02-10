#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
layout (std140, set = 0, binding = 0) uniform buf {
    mat4 mvp;
} ubuf;
layout (location = 0) in vec4 pos;
layout (location = 1) in vec2 inTexCoords;
layout (location = 0) out vec2 outTexCoords;
out gl_PerVertex { 
    vec4 gl_Position;
};
void main() {
   gl_Position = ubuf.mvp * pos;
   outTexCoords = inTexCoords;
   // GL->VK conventions
   gl_Position.y = -gl_Position.y;
   gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
}