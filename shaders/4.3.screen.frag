#version 430 core

out vec4 FragColor;

uniform sampler2D screen;

uniform int screenWidth = 800;
uniform int screenHeight = 600;

void main() {
    vec2 uv = gl_FragCoord.xy / vec2(screenWidth, screenHeight);
    vec4 c = texture(screen, uv);
    FragColor = c;
}