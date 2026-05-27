#version 330

layout (location = 0) out vec4 outColor;

in vec3 fNormal;

void main() {

	outColor = vec4(abs(fNormal), 1.0);
}