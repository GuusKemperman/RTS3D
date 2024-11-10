#version 310 es

layout(location = 0) in mediump vec3 vertexPosition;
layout(location = 1) in mediump vec3 vertexNormal;
layout(location = 2) in mediump vec2 vertexUV;
layout(location = 3) in highp mat4 instanceModel;

out mediump vec2 fragUV;
out mediump vec3 fragNormal;
out mediump vec3 fragPos;

uniform highp mat4 viewProjection;

void main()
{
	gl_Position = viewProjection * instanceModel * vec4(vertexPosition,1);

	fragNormal = normalize(vec3(instanceModel * vec4(vertexNormal, 0.0)));
	fragUV = vertexUV;
	fragPos = vec3(instanceModel * vec4(vertexPosition, 1.0));
}
