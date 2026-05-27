#version 330 core

in vec2 fragUV;
out vec4 outColor;

uniform sampler2D debugTexture;
uniform bool remapDepth;
uniform float depthMin;
uniform float depthMax;

void main()
{
    vec4 value = texture(debugTexture, fragUV);
    if (remapDepth)
    {
        float width = max(depthMax - depthMin, 0.000001);
        float depth = clamp((value.r - depthMin) / width, 0.0, 1.0);
        outColor = vec4(vec3(depth), 1.0);
        return;
    }

    outColor = vec4(value.rgb, 1.0);
}
