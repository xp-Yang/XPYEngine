struct DirectionalLight
{
    vec3 direction;
    vec3 color;
};

struct PointLight
{
    vec3 position;
    vec3 color;
    float radius;
};

uniform DirectionalLight directionalLight;
uniform sampler2D shadow_map;
uniform mat4 lightSpaceMatrix;

// TODO shader里怎么用动态数组
// 注意：降为 5 是为了给 IBL 让出纹理单元（见 IBL 方案“纹理单元预算”）。
// 必须与 C++ 端 MAX_CUBE_SHADOW_MAP_COUNT 保持一致。
const int MAX_POINT_LIGHTS_COUNT = 5;
uniform int point_lights_size;
uniform PointLight pointLights[MAX_POINT_LIGHTS_COUNT];
uniform samplerCube cube_shadow_maps[MAX_POINT_LIGHTS_COUNT];

// IBL（Image-Based Lighting）环境光资源。iblEnable 关闭时退回常量 ambient。
uniform bool iblEnable;
uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D brdfLUT;

const float gamma = 2.2;

float ShadowCalculation(vec4 fragPosLightSpace, sampler2D shadow_map)
{
    // 1.还在裁剪空间，执行透视除法，变换到NDC空间
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // 2.变换到[0,1]的范围, 便于采样
    projCoords = projCoords * 0.5 + 0.5;
    // if(projCoords.z > 1.0)
    //     return 1.0;

    float bias = 0.005;
    // float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
    float closestDepth = texture(shadow_map, projCoords.xy).r;
    // 检查当前片段是否在阴影中
    float shadowFactor = projCoords.z - closestDepth > bias ? 0.5 : 1.0;

    return shadowFactor;
}

float PointLightAttenuation(float distance, float radius)
{
    float k_quadratic = 0.2 / radius;
    float attenuation = step(distance, radius) * (1.0 / (1.0 + k_quadratic * distance * distance));
    return attenuation;
}

float OmnidirectionalShadowCalculation(vec3 lightToFrag, samplerCube cube_map, float far_plane)
{
    float distance = length(lightToFrag);
    if (distance > far_plane)
        return 1.0;
    float closestDepth = texture(cube_map, lightToFrag).r;
    float worldDepth = closestDepth * far_plane;
    float bias = 0.5;
    float shadowFactor = distance - worldDepth > bias ? 0 : 1.0;

    return shadowFactor;
}

vec3 ToneMapping(vec3 color, float exposure)
{
    vec3 res = vec3(1.0) - exp(-color * exposure);
    return res;
}

vec3 GammaCorrection(vec3 color)
{
    vec3 res = pow(color, vec3(1.0 / gamma));
    return res;
}
#line 1