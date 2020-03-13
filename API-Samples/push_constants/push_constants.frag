#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
layout(push_constant) uniform pushBlock {
    int iFoo;
    float fBar;
} pushConstantsBlock;
layout (location = 0) in vec2 inTexCoords;
layout (location = 0) out vec4 outColor;
void main() {

    vec4 green = vec4(0.0, 1.0, 0.0, 1.0);
    vec4 red   = vec4(1.0, 0.0, 0.0, 1.0);

    // Start with passing color
    vec4 resColor = green;

    // See if we've read in the correct push constants
    if (pushConstantsBlock.iFoo != 2)
        resColor = red;
    if (pushConstantsBlock.fBar != 1.0f)
        resColor = red;

    // Create a border to see the cube more easily
   if (inTexCoords.x < 0.01 || inTexCoords.x > 0.99)
       resColor *= vec4(0.1, 0.1, 0.1, 1.0);
   if (inTexCoords.y < 0.01 || inTexCoords.y > 0.99)
       resColor *= vec4(0.1, 0.1, 0.1, 1.0);
   outColor = resColor;
}