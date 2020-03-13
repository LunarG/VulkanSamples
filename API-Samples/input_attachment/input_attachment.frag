#version 450
layout (input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput
myInputAttachment;
layout (location = 0) out vec4 outColor;
void main() {
   outColor = subpassLoad(myInputAttachment);
}