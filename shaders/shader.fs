#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec3 Color;

uniform sampler2D texture1;
uniform sampler2D texture2;

void main() {
	FragColor = vec4(Color, 1.0f);
}