#version 310 es

// Interpolated values from the vertex shaders
in mediump vec2 fragUV;
in mediump vec3 fragNormal;
in mediump vec3 fragPos;

// Ouput data
out mediump vec3 color;

// Lighting
uniform mediump vec3 lightColor;
uniform mediump vec3 ambientLight;
uniform mediump vec3 lightDirection;
uniform mediump vec3 cameraPos;
uniform mediump float specularStrength;

uniform sampler2D sampler;

void main()
{
	mediump vec3 baseColor = texture( sampler, fragUV ).rgb;

	mediump float diff = max(dot(lightDirection, fragNormal), 0.0);
	mediump vec3 diffuse = diff * lightColor;

    mediump vec3 viewDir = normalize(cameraPos - fragPos);
    mediump vec3 reflectDir = reflect(-lightDirection, fragNormal);  
    mediump float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    mediump vec3 specular = specularStrength * spec * lightColor;  

	color = (ambientLight + diffuse + specular) * baseColor;
}