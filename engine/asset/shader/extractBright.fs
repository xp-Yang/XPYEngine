#version 330 core

in vec2 fragUV;

uniform sampler2D Texture;

out vec4 outColor;

void main() {

    vec4 color =  texture(Texture, fragUV);
    float brightness = dot(color.rgb, vec3(0.2126, 0.7152, 0.0722));
    outColor = step(1.5, brightness) * vec4(color.rgb, 1.0);
}