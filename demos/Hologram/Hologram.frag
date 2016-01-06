#version 310 es

precision highp float;

in vec4 color;
layout(location = 0) out vec4 fragcolor;

void main()
{
	fragcolor = color;
}
