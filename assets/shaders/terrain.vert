#version 310 es

layout(location = 0) in mediump vec3 vertexPosition;
layout(location = 1) in mediump vec3 vertexNormal;
layout(location = 2) in mediump vec2 vertexUV;

out mediump vec2 fragUV;
out mediump vec3 fragNormal;
out highp vec3 fragPos;

uniform mat4 MVP;
uniform mat4 modelMatrix;

void main()
{
	gl_Position = MVP * vec4(vertexPosition, 1.0);
	
	// The terrain has not been rotated or scaled, just pass the normal without adjusting for the model matrix.
	fragNormal = vertexNormal;
	fragUV = vertexUV;
	fragPos = vec3(modelMatrix * vec4(vertexPosition, 1.0));
}
