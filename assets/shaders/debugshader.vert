#version 310 es

layout(location = 0) in mediump vec3 vertexPosition;
layout(location = 1) in mediump vec3 vertexColor;

uniform mediump mat4 MVP;

out mediump vec3 fragColor;

void main()
{
	gl_Position =  MVP * vec4(vertexPosition, 1);

	fragColor = vertexColor;
}

