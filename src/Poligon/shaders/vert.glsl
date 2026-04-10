#version 450 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(binding = 0) uniform UBO {
    mat4 model; mat4 view; mat4 proj;
    vec3 lightPos; vec3 lightColor; vec3 viewPos;
} ubo;
layout(location = 0) out vec3 FragPos;
layout(location = 1) out vec3 Normal;
void main() {
    vec4 worldPos = ubo.model * vec4(aPos, 1.0);
    FragPos = worldPos.xyz;
    Normal = mat3(transpose(inverse(ubo.model))) * aNormal;
    gl_Position = ubo.proj * ubo.view * worldPos;
}