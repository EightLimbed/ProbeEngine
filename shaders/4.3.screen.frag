#version 440 core

out vec4 FragColor;

layout(binding = 0) uniform sampler2D colorMap; // color map
layout(binding = 1) uniform sampler2D depthMap; // depth and normal map
layout(binding = 2) uniform sampler3D lightMap; // lighting map

uniform int screenWidth = 1;
uniform int screenHeight = 1;
uniform int lightFidelity = 1;
uniform int lightSamples = 1;
uniform int smoothing = 1;

//include shaderheaders/4.3.constantsH.comp

// averages lighting data from multiple passes. minimum two passes.
vec4 averagePasses(vec2 uv) {
    // takes advantage of linear filtering to do one less sample (samples between each pixel)
    vec4 color = vec4(0.0);
    const float s = 1/float(lightSamples);
    for (int i = 0; i<lightSamples-1; i++) {
        float w = (0.5+float(i))*s;
        color += texture(lightMap, vec3(uv, w));
    }
    return color*s;
}

// smooths lighting with kernel based on normals
// size is in light texels, every pixel within those light texels is sampled
vec4 smoothLighting(vec2 uv, int size) {
    float accumulate = 0.0;
    vec4 color = vec4(0.0);
    vec4 reference = texture(depthMap, uv); // reference depth and normal value
    for (int x = 0; x<size; x++)
    for (int y = 0; y<size; y++) {
        // get updated uvs and data
        vec2 uuv = uv+(vec2(x,y)-float(size)*0.25)*float(lightFidelity)/vec2(screenWidth, screenHeight);
        vec4 nDepth = texture(depthMap, uv); // depth and normal value to be compared

        // weight based on depth as well
        float dWeight = 1.0-min(abs(reference.w-nDepth.w),1.0);

        // smooth with weight based on normal
        float nWeight = 1.0;//dot(nDepth.xyz, reference.xyz);
        color += nWeight*dWeight*averagePasses(uuv); // gets weighted light
        accumulate += nWeight*dWeight;
    }
    return color/accumulate;
}

void main() {
    vec2 uv = gl_FragCoord.xy / vec2(screenWidth, screenHeight);
    vec4 color = texture(colorMap, uv);
    vec4 depth = texture(depthMap, uv);
    vec4 light = (smoothing==1) ? smoothLighting(uv, 4) : averagePasses(uv);
    vec3 normal = depth.xyz;

    float fog = pow(depth.w*1.1/renderDist,15.0);

    FragColor = (depth.w<renderDist) ? mix(light*color,sky,fog) : sky;
}