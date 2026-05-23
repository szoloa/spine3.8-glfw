#version 330 core

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec4 aColor;

out vec2 vUV;
out vec4 vColor;

uniform vec2 screenSize;

void main() {
    vec2 pos = aPos / screenSize;

    gl_Position = vec4(pos, 0.0, 1.0);

    vUV = aUV;
    vColor = aColor;
}
