#version 450

layout(location = 0) in vec2 fragPos;
layout(location = 0) out vec4 outColor;

uniform vec3 highlightColor = vec3(1.0, 0.0, 0.0);
uniform float highlightIntensity = 1.0;

void main() {
    outColor = vec4(highlightColor, 1.0);
}