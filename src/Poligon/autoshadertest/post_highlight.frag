#version 450
layout(binding = 0) uniform sampler2D sceneImage;
layout(binding = 1) uniform sampler2D idImage;
layout(binding = 2) uniform UniformBuffer {
    uint targetID;
    uint highlightMode;
    float outlineWidth;
    float outlineStrength;
} ubo;
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 outColor;
bool hasNeighborWithID(sampler2D idTex, vec2 uv, uint targetID, float stepX, float stepY) {
    vec2 offsets[8] = vec2[](
        vec2(-stepX, -stepY), vec2(0, -stepY), vec2(stepX, -stepY),
        vec2(-stepX, 0),                     vec2(stepX, 0),
        vec2(-stepX, stepY), vec2(0, stepY), vec2(stepX, stepY)
    );
    for (int i = 0; i < 8; i++) {
        uint id = uint(texture(idTex, uv + offsets[i]).r);
        if (id == targetID) return true;
    }
    return false;
}
void main() {
    vec4 sceneColor = texture(sceneImage, vTexCoord);
    uint id = uint(texture(idImage, vTexCoord).r);
    if (id == ubo.targetID) {
        if (ubo.highlightMode == 0) {
            outColor = mix(sceneColor, vec4(1.0, 0.0, 0.0, 1.0), 0.6);
            return;
        } else if (ubo.highlightMode == 2) {
            outColor = mix(sceneColor, vec4(0.0, 1.0, 0.0, 1.0), 0.5);
            return;
        }
    }
    if (ubo.highlightMode == 1 && id != ubo.targetID) {
        vec2 texelSize = 1.0 / textureSize(idImage, 0);
        float stepX = texelSize.x * ubo.outlineWidth;
        float stepY = texelSize.y * ubo.outlineWidth;
        if (hasNeighborWithID(idImage, vTexCoord, ubo.targetID, stepX, stepY)) {
            outColor = mix(sceneColor, vec4(1.0, 0.5, 0.0, 1.0), ubo.outlineStrength);
            return;
        }
    }
    outColor = sceneColor;
}