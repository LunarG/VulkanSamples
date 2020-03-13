#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
// Note, set = 0 here
layout (std140, set = 0, binding = 0) uniform bufferVals {
    mat4 mvp;
} myBufferVals;
// And set = 1 here
layout (set = 1, binding = 0) uniform sampler2D surface;
layout (location = 0) in vec4 pos;
layout (location = 1) in vec2 inTexCoords;
layout (location = 0) out vec4 outColor;
layout (location = 1) out vec2 outTexCoords;
void main() {
   outColor = texture(surface, vec2(0.0));
   outTexCoords = inTexCoords;
   gl_Position = myBufferVals.mvp * pos;
}