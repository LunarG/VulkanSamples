// This shader renders a simple fullscreen quad using the VS alone
#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
layout (location = 0) out vec4 outColor;
void main() {
   outColor = vec4(1.0f, 0.1f, 0.1f, 0.5f);
   const vec4 verts[4] = vec4[4](vec4(-1.0, -1.0, 0.5, 1.0),
                                 vec4( 1.0, -1.0, 0.5, 1.0),
                                 vec4(-1.0,  1.0, 0.5, 1.0),
                                 vec4( 1.0,  1.0, 0.5, 1.0));

   gl_Position = verts[gl_VertexIndex];
}