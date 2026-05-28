#version 330 core

in vec2 fragUV;

uniform sampler2D sceneColor;
uniform sampler2D bloomTexture;
uniform float bloomIntensity;

out vec4 FragColor;

void main()
{
    vec3 scene = texture(sceneColor, fragUV).rgb;
    vec3 bloom = texture(bloomTexture, fragUV).rgb;
    FragColor = vec4(scene + bloom * bloomIntensity, 1.0);
}