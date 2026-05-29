#version 330 core

in vec2 fragUV;
out float FragColor;

uniform sampler2D gPosition;   // 世界空间位置
uniform sampler2D gNormal;     // 世界空间法线

uniform mat4 view;
uniform mat4 projection;

const int MAX_KERNEL_SIZE = 64;
uniform vec3 samples[MAX_KERNEL_SIZE];
uniform int  kernelSize;
uniform float radius;
uniform float bias;
uniform float power;
uniform vec2 noiseScale; // 屏幕分辨率，用于程序化噪声缩放

// 程序化 hash 噪声，返回切线平面内的随机旋转向量（z=0）。
vec3 hashRotation(vec2 p)
{
    float n = fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
    float angle = n * 6.2831853;
    return vec3(cos(angle), sin(angle), 0.0);
}

void main()
{
    vec4 worldPos = texture(gPosition, fragUV);
    // 背景像素（GBuffer 清空，alpha=0 或法线为零）直接输出无遮蔽。
    vec3 worldNormal = texture(gNormal, fragUV).xyz;
    if (worldPos.a < 0.5 && dot(worldNormal, worldNormal) < 1e-5)
    {
        FragColor = 1.0;
        return;
    }

    vec3 fragPos = (view * vec4(worldPos.xyz, 1.0)).xyz;          // 视空间位置
    vec3 normal  = normalize(mat3(view) * worldNormal);           // 视空间法线
    vec3 randomVec = hashRotation(fragUV * noiseScale);

    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);

    float occlusion = 0.0;
    for (int i = 0; i < kernelSize; ++i)
    {
        vec3 samplePos = fragPos + TBN * samples[i] * radius;     // 视空间采样点

        vec4 offset = projection * vec4(samplePos, 1.0);
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5 + 0.5;                      // 到 [0,1] 屏幕 UV

        if (offset.x < 0.0 || offset.x > 1.0 || offset.y < 0.0 || offset.y > 1.0)
            continue;

        vec4 sampleWorld = texture(gPosition, offset.xy);
        float sampleDepth = (view * vec4(sampleWorld.xyz, 1.0)).z;

        float rangeCheck = smoothstep(0.0, 1.0, radius / max(abs(fragPos.z - sampleDepth), 1e-4));
        occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
    }

    float ao = 1.0 - occlusion / float(kernelSize);
    FragColor = pow(clamp(ao, 0.0, 1.0), power);
}
