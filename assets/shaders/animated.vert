#version 310 es

layout(location = 0) in mediump vec3 vertexPosition;
layout(location = 1) in mediump vec3 vertexNormal;
layout(location = 2) in mediump vec2 vertexUV;

layout(location = 3) in ivec4 boneIds;
layout(location = 4) in mediump vec4 weights;

uniform mediump mat4 viewProjection;
uniform mediump mat4 model;

const int MAX_BONES = 100;
uniform mediump mat4 finalBonesMatrices[MAX_BONES];
	
out mediump vec2 fragUV;
out mediump vec3 fragNormal;
out mediump vec3 fragPos;

void main()
{
    vec4 totalPosition = vec4(0.0f);

    mediump mat4 modelWithBoneWeights = model * (finalBonesMatrices[boneIds[0]] * weights[0]
        + finalBonesMatrices[boneIds[1]] * weights[1]
        + finalBonesMatrices[boneIds[2]] * weights[2]
        + finalBonesMatrices[boneIds[3]] * weights[3]);

    gl_Position = viewProjection * modelWithBoneWeights * vec4(vertexPosition, 1.0f);

    fragUV = vertexUV;
    mediump mat3 normalMatrix = transpose(inverse(mat3(modelWithBoneWeights)));
    fragNormal = normalize(normalMatrix * vertexNormal);
	fragPos = vec3(model * vec4(vertexPosition, 1.0));
}