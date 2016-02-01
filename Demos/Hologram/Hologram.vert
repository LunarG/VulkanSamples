#version 310 es

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_normal;

layout(std140, set = 0, binding = 0) buffer param_block {
	mat4 mvp;
} params;

out vec3 color;

void main()
{
	vec4 pos = params.mvp * vec4(in_pos, 1.0);

	gl_Position = pos;
	color = in_normal;
}
