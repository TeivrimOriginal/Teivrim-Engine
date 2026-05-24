#version 450
layout(location = 0) in flat uint inObjectID;
layout(location = 0) out uvec4 outColor;
void main() {
    outColor = uvec4(inObjectID, 0, 0, 1);
}