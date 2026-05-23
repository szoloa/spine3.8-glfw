#version 330 core

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec4 aColor;

out vec2 vUV;
out vec4 vColor;

void main() {
    float x = aPos.x / 2000.0;
    float y = aPos.y / 1500.0;

    gl_Position = vec4(x, y, -1.0, 1.0);

    vUV = aUV;
    vColor = aColor;
}
