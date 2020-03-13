#version 450
layout (location = 0) in vec2 texcoord;
layout (location = 0) out vec4 outColor;
void main() {
   outColor = vec4(texcoord.x, texcoord.y,
       1.0 - texcoord.x - texcoord.y, 1.0f);
}