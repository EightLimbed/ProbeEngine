#version 440 core

out vec4 FragColor;

layout(binding = 0) uniform sampler2D colorMap; // color map
layout(binding = 1) uniform sampler2D depthMap; // depth and normal map
layout(binding = 2) uniform sampler2D lightMap; // lighting map

uniform int screenWidth = 1;
uniform int screenHeight = 1;

//include shaderheaders/4.3.constantsH.comp

void main() {
    vec2 uv = gl_FragCoord.xy / vec2(screenWidth, screenHeight);
    vec4 color = texture(colorMap, uv);
    vec4 depth = texture(depthMap, uv);
    vec3 normal = depth.xyz;
    float shadow = max(dot(normal,vec3(1.0)),0.3); // basic shading
    float fog = pow(depth.w*1.1/renderDist,15.0);

    FragColor = (depth.w<renderDist) ? mix(color*shadow,sky,fog) : sky;
}