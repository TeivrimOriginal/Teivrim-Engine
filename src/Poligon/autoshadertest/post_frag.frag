#version 450
layout(binding = 0) uniform sampler2D idTexture;  // sampler2D вместо usampler2D
layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PushConstants {
    uint selectedID;
    vec3 outlineColor;
    vec2 texelSize;
    int thickness;
} pc;

void main() {
    float centerVal = texture(idTexture, fragTexCoord).r;
    uint centerID = uint(centerVal * 255.0);
    
    if (centerID == pc.selectedID) {
        outColor = vec4(pc.outlineColor, 1.0);
    } else {
        outColor = vec4(0.0, 0.0, 0.0, 0.0);
    }
}
