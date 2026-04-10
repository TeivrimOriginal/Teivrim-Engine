#version 450 core
layout(location = 0) in vec3 FragPos;
layout(location = 1) in vec3 Normal;
layout(location = 0) out vec4 outColor;
void main() {
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(vec3(2.0, 5.0, 2.0) - FragPos);
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * vec3(1.0);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * vec3(1.0);
    float specularStrength = 0.5;
    vec3 viewDir = normalize(vec3(0.0, 0.0, 5.0) - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = specularStrength * spec * vec3(1.0);
    vec3 result = (ambient + diffuse + specular) * vec3(0.8, 0.6, 0.4);
    outColor = vec4(result, 1.0);
}