#version 450
layout(push_constant) uniform PushConstants { mat4 mvp; } pc;
layout(location = 0) in vec3 inPosition;
layout(location = 0) out vec4 fragColor;
void main() {
    gl_Position = pc.mvp * vec4(inPosition, 1.0);
    fragColor = vec4(1.0, 0.5, 0.0, 1.0);
}
