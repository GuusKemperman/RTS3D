#version 310 es

// Interpolated values from the vertex shaders
in mediump vec2 fragUV;
in mediump vec3 fragNormal;
in highp vec3 fragPos;

// Ouput data
out mediump vec3 color;

// Lighting
uniform mediump vec3 lightColor;
uniform mediump vec3 ambientLight;
uniform mediump vec3 lightDirection;
uniform mediump vec3 cameraPos;
uniform mediump float specularStrength;

uniform sampler2D flatSampler;
uniform sampler2D steepSampler;

// Used for making it less obvious that the texture is repeating, see https://iquilezles.org/articles/texturerepetition/
const mediump float amountOfVirtualVariations = 8.0;
const mediump float noiseRoughness = 0.0005f;

// If we want to optimize, we could hardcode the results of this function for each virtual variation so we don't have to run this twice for each fragment.
mediump vec2 GenerateOffset(mediump float virtualVariationIndex)
{
    // Can replace with any other hash.
    return .5f * sin(vec2(4.0, .67) * virtualVariationIndex); 
}

mediump vec4 CalculateNonTileUvs(mediump vec2 originalUv)
{
	// We store the noise in the alpha of the flat sampler
    mediump float noiseSample = texture( flatSampler, noiseRoughness * vec2(fragPos.xz)).a;
        
    mediump float l = noiseSample * amountOfVirtualVariations;
    mediump float f = fract(l);
    
    mediump float indexA = floor(l);
    mediump float indexB = indexA + 1.0;

    mediump vec2 offsetA = GenerateOffset(indexA);
    mediump vec2 offsetB = GenerateOffset(indexB);

    mediump vec2 uvA = originalUv + offsetA;
    mediump vec2 uvB = originalUv + offsetB;

    return vec4(uvA.x, uvA.y, uvB.x, uvB.y);
}

mediump vec3 MixColors(mediump vec4 uvs, sampler2D sampler, mediump vec2 duvdx, mediump vec2 duvdy)
{
    mediump vec3 colA = textureGrad( sampler, uvs.xy, duvdx, duvdy ).xyz;
    mediump vec3 colB = textureGrad( sampler, uvs.zw, duvdx, duvdy ).xyz;
    return mix( colA, colB, .5);   
}

void main()
{
    mediump vec4 newUvs = CalculateNonTileUvs(fragUV);

    // We'll be changing the final UV, but we should use our original UV to determine which mipmap to we need.
    mediump vec2 duvdx = dFdx( fragUV );
    mediump vec2 duvdy = dFdy( fragUV );

    mediump vec3 flatCol = MixColors(newUvs, flatSampler, duvdx, duvdy);
    mediump vec3 steepCol = MixColors(newUvs, steepSampler, duvdx, duvdy);

    mediump float steepness = pow(dot(fragNormal, vec3(0.0, 1.0, 0.0)), 4.0);

	mediump vec3 baseColor = mix(steepCol, flatCol, steepness);

	mediump float diff = max(dot(lightDirection, fragNormal), 0.0);
	mediump vec3 diffuse = diff * lightColor;

    mediump vec3 viewDir = normalize(cameraPos - fragPos);
    mediump vec3 reflectDir = reflect(-lightDirection, fragNormal);  
    mediump float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    mediump vec3 specular = specularStrength * spec * lightColor;  

	color = (ambientLight + diffuse + specular) * baseColor;
}