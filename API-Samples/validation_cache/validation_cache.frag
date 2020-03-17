// The fragment shader contains a 32-bit integer constant (tweak_value)
// which we can search for in the compiled SPIRV and replace with new
// values to generate "different" shaders.
#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
const uint tweak_value = 0xdeadbeef;
layout (binding = 1) uniform sampler2D tex;
layout (location = 0) in vec2 texcoord;
layout (location = 0) out vec4 outColor;
void main() {
   outColor = textureLod(tex, texcoord, 0.0);
   outColor.a = float(tweak_value);
}