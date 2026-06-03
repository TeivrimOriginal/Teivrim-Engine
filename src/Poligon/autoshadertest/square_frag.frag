#version 450
layout(binding = 0) uniform sampler2D sceneImage;
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 outColor;
void main() {
    vec2 center = vec2(0.5, 0.5);
    float size = 0.1;
    if (abs(vTexCoord.x - 0.5) < size && abs(vTexCoord.y - 0.5) < size) {
        outColor = vec4(1.0, 0.0, 0.0, 1.0);
    } else {
        outColor = texture(sceneImage, vTexCoord);
    }
}