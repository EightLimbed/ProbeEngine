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
    vec4 c = texture(depthMap, uv);
    FragColor = c*(1-c.w/renderDist);
}