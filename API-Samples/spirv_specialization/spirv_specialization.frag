#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
layout (binding = 1) uniform sampler2D tex;
layout (location = 0) in vec2 texcoord;
layout (location = 0) out vec4 outColor;
layout (constant_id = 5) const bool drawUserColor = false;
layout (constant_id = 7) const float r = 0.0f;
layout (constant_id = 8) const float g = 0.0f;
layout (constant_id = 9) const float b = 0.0f;
void main() {
   if (drawUserColor)
      outColor = vec4(r, g, b, 1.0);
   else
      outColor = textureLod(tex, texcoord, 0.0);
}