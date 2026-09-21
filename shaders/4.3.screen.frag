#version 440 core

out vec4 FragColor;

layout(binding = 0) uniform sampler2D colorMap; // color map
layout(binding = 1) uniform sampler2D depthMap; // depth and normal map
layout(binding = 2) uniform sampler3D lightMap; // lighting map

uniform int screenWidth = 1;
uniform int screenHeight = 1;
uniform int lightSamples = 1; // light samples per frame
uniform int lightFrames = 1; // light frames averaged

//include shaderheaders/4.3.constantsH.comp

// averages lighting data from multiple passes and frames. minimum two passes.
vec4 averagePasses(vec2 uv) {
    // takes advantage of linear filtering to do one less sample (samples between each pixel)
    vec4 light = vec4(0.0);
    const float s = 1/float(lightSamples*lightFrames);
    for (int i = 0; i<(lightSamples*lightFrames)-1; i++) {
        float w = (0.5+float(i))*s;
        light += texture(lightMap, vec3(uv, w));
    }
    return light*s;
}

// smooths lighting with radial kernel that stops at edges
vec4 smoothLighting(vec2 uv, float angles, float radius, float jump) {
    float accumulate = 0.0; // sum of weights for averaging color
    vec4 color = vec4(0.0); // accumulated color
    vec4 reference = texture(depthMap, uv); // reference depth and normal value
    vec4 refColor = averagePasses(uv);
    float invangles = 1.0/angles; // inverse angles to avoid excess division
    // each angle
    for (float c = 0.0; c<angles; c+=1.0) {
        // gets angle
        float angle = TwoPI*invangles*c;
        // gets vector direction from angle, then fits to screen
        vec2 dir = vec2(cos(angle),sin(angle));
        vec2 udir = dir/vec2(screenWidth, screenHeight);

        // radiate from angles
        for (float r = jump; r < radius; r+=jump) {
            // get updated uvs
            vec2 uuv = uv+udir*r;

            // get depth and normal
            vec4 nDepth = texture(depthMap, uv); // depth and normal value to be compared
            float dWeight = 1.0-abs(reference.w-nDepth.w)*4;
            float nWeight = dot(nDepth.xyz, reference.xyz);

            // if edge too harsh, break
            if (dWeight<0.0||nWeight<0.5) break;

            // accumulate and average color based on normals
            color += averagePasses(uuv)*nWeight;
            accumulate += nWeight;
        }
    }
    return color/accumulate;
}

void main() {
    vec2 uv = gl_FragCoord.xy / vec2(screenWidth, screenHeight);
    vec4 color = texture(colorMap, uv);
    vec4 depth = texture(depthMap, uv);
    vec4 light = smoothLighting(uv, 10, 8, 4);//averagePasses(uv);
    vec3 normal = depth.xyz;

    float fog = pow(depth.w*1.1/renderDist,15.0);

    FragColor = (depth.w<renderDist) ? mix(light*color,sky,fog) : sky;
}