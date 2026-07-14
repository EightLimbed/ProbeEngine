#version 440 core

out vec4 FragColor;

uniform sampler2D screen;

uniform int screenWidth = 1;
uniform int screenHeight = 1;

void main() {
    vec2 uv = gl_FragCoord.xy / vec2(screenWidth, screenHeight);
    vec4 c = texture(screen, uv);
    FragColor = c;//vec4(uv,1.0,1.0);
}