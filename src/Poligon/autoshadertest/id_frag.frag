#version 450
layout(location = 0) in flat uint inObjectID;
layout(location = 0) out vec4 outColor;  // Меняем на vec4
void main() {
    // Конвертируем uint в float (0-255 -> 0-1)
    outColor = vec4(float(inObjectID) / 255.0, 0.0, 0.0, 1.0);
}
