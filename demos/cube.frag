/*
 * Fragment shader for cube demo
 */
#version 140
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
layout (binding = 0) uniform sampler2D tex;

layout (location = 0) in vec4 texcoord;
void main() {
   gl_FragColor = texture(tex, texcoord.xy);
}
