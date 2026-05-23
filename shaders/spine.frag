#version 330 core

in vec2 vUV;
in vec4 vColor;

uniform sampler2D tex;

out vec4 FragColor;

void main() {
    FragColor = texture(tex, vUV) * vColor;
}
