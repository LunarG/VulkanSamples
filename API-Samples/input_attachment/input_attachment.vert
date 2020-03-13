#version 450
vec2 vertices[3];
void main() {
    vertices[0] = vec2(-1.0, -1.0);
    vertices[1] = vec2( 1.0, -1.0);
    vertices[2] = vec2( 0.0,  1.0);
    gl_Position = vec4(vertices[gl_VertexIndex % 3], 0.0, 1.0);
}