#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
layout (set = 0, binding = 1) uniform texture2D tex;
layout (set = 0, binding = 2) uniform sampler samp;
layout (location = 0) in vec2 inTexCoords;
layout (location = 0) out vec4 outColor;
void main() {

    // Combine the selected texture with sampler as a parameter
    vec4 resColor = texture(sampler2D(tex, samp), inTexCoords);

    // Create a border to see the cube more easily
   if (inTexCoords.x < 0.01 || inTexCoords.x > 0.99)
       resColor *= vec4(0.1, 0.1, 0.1, 1.0);
   if (inTexCoords.y < 0.01 || inTexCoords.y > 0.99)
       resColor *= vec4(0.1, 0.1, 0.1, 1.0);
   outColor = resColor;
}