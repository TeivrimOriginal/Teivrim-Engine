#version 450
layout(location = 0) in flat uint inObjectID;
layout(location = 0) out uint outColor;
void main() {
    outColor = inObjectID;
}
