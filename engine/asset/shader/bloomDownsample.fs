#version 330 core

in vec2 fragUV;

uniform sampler2D source;
uniform vec2 texelSize;
uniform bool applyThreshold;
uniform float threshold;
uniform float softKnee;

out vec4 FragColor;

// 13-tap downsample filter (Jimenez 2014 / Call of Duty).
// Samples a 4x4 source texel region using 5 overlapping bilinear taps
// to suppress firefly artifacts during progressive downsampling.
vec3 downsample13Tap(vec2 uv, vec2 ts)
{
    vec3 A = texture(source, uv + ts * vec2(-1.0,  1.0)).rgb;
    vec3 B = texture(source, uv + ts * vec2( 1.0,  1.0)).rgb;
    vec3 C = texture(source, uv + ts * vec2(-1.0, -1.0)).rgb;
    vec3 D = texture(source, uv + ts * vec2( 1.0, -1.0)).rgb;
    vec3 E = texture(source, uv).rgb;

    vec3 F = texture(source, uv + ts * vec2(-1.0,  0.0)).rgb;
    vec3 G = texture(source, uv + ts * vec2( 1.0,  0.0)).rgb;
    vec3 H = texture(source, uv + ts * vec2( 0.0,  1.0)).rgb;
    vec3 I = texture(source, uv + ts * vec2( 0.0, -1.0)).rgb;

    vec3 J = texture(source, uv + ts * vec2(-2.0,  2.0)).rgb;
    vec3 K = texture(source, uv + ts * vec2( 2.0,  2.0)).rgb;
    vec3 L = texture(source, uv + ts * vec2(-2.0, -2.0)).rgb;
    vec3 M = texture(source, uv + ts * vec2( 2.0, -2.0)).rgb;

    vec3 result = E * 0.125;
    result += (A + B + C + D) * 0.125;
    result += (F + G + H + I) * 0.0625;
    result += (J + K + L + M) * 0.03125;

    return result;
}

// Soft knee threshold via smoothstep: avoids division by brightness which
// amplifies float16 quantization into visible banding on smooth gradients.
vec3 applyBrightnessThreshold(vec3 color, float thresh, float knee)
{
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
    float weight = smoothstep(thresh - knee, thresh + knee, brightness);
    return color * weight;
}

void main()
{
    vec3 color = downsample13Tap(fragUV, texelSize);

    if (applyThreshold)
        color = applyBrightnessThreshold(color, threshold, threshold * softKnee);

    FragColor = vec4(color, 1.0);
}
