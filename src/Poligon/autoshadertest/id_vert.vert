#version 450
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec2 inTexCoord;
layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 view;
    mat4 proj;
    uint objectID;
} pc;
layout(location = 0) out uint outObjectID;
void main() {
    gl_Position = pc.proj * pc.view * pc.model * vec4(inPosition, 1.0);
    outObjectID = pc.objectID;
}
