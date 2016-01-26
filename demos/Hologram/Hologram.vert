#version 310 es

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec4 in_color;

layout(set = 0, binding = 0) buffer transformation {
	mat4 mvp;
} matrices;

out vec4 color;

void main()
{
	vec4 pos = matrices.mvp * vec4(in_pos, 1.0);

	gl_Position = pos;
	color = in_color;
}
