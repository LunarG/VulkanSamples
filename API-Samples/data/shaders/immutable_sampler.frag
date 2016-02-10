#version 140
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
layout (set = 0, binding = 1) uniform sampler2D surface;
layout (location = 0) in vec2 inTexCoords;
layout (location = 0) out vec4 outColor;
void main() {
    outColor = texture(surface, inTexCoords);
}