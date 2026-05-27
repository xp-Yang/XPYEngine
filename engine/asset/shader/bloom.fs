#version 330 core

#include "common.h"

in vec2 fragUV;

uniform sampler2D Texture;
//uniform sampler2DMS Texture;
uniform sampler2D bloomMap;

out vec4 FragColor;

void main()
{
    vec3 sceneColor = texture(Texture, fragUV).rgb;      

    vec3 bloomColor = texture(bloomMap, fragUV).rgb;
    sceneColor += bloomColor; // additive blending

    FragColor = vec4(sceneColor, 1.0);
}