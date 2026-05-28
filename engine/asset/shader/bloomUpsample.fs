#version 330 core

in vec2 fragUV;

uniform sampler2D source;
uniform sampler2D destination;
uniform vec2 texelSize;
uniform float blendFactor;

out vec4 FragColor;

// 3x3 tent filter upsample for smoother blending than raw bilinear.
vec3 upsample9Tap(vec2 uv, vec2 ts)
{
    vec3 a = texture(source, uv + vec2(-ts.x,  ts.y)).rgb;
    vec3 b = texture(source, uv + vec2(  0.0,  ts.y)).rgb;
    vec3 c = texture(source, uv + vec2( ts.x,  ts.y)).rgb;

    vec3 d = texture(source, uv + vec2(-ts.x,   0.0)).rgb;
    vec3 e = texture(source, uv).rgb;
    vec3 f = texture(source, uv + vec2( ts.x,   0.0)).rgb;

    vec3 g = texture(source, uv + vec2(-ts.x, -ts.y)).rgb;
    vec3 h = texture(source, uv + vec2(  0.0, -ts.y)).rgb;
    vec3 i = texture(source, uv + vec2( ts.x, -ts.y)).rgb;

    return (a + c + g + i) * (1.0 / 16.0)
         + (b + d + f + h) * (2.0 / 16.0)
         + e               * (4.0 / 16.0);
}

void main()
{
    vec3 upsampled = upsample9Tap(fragUV, texelSize);
    vec3 existing  = texture(destination, fragUV).rgb;
    FragColor = vec4(mix(existing, upsampled, blendFactor), 1.0);
}
