#version 450
layout(binding = 0) uniform usampler2D idTexture;
layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PushConstants {
    uint selectedID;
    vec3 outlineColor;
    vec2 texelSize;
} pc;

void main() {
    uint centerID = texture(idTexture, fragTexCoord).r;
    
    if (centerID == pc.selectedID) {
        outColor = vec4(pc.outlineColor, 1.0);
    } else {
        outColor = vec4(0.0, 0.0, 0.0, 0.0);
    }
}
