#version 330 core

#include "common.h"
#include "BRDF.h"

in vec2 fragUV;

uniform sampler2D gPosition;
uniform sampler2D gNormal;

uniform sampler2D gAlbedo;
uniform sampler2D gMetallic;
uniform sampler2D gRoughness;
uniform sampler2D gAo;

uniform vec3 cameraPos;

layout (location = 0) out vec4 FragColor;

void main()
{             
    // retrieve data from gbuffer
    vec3 Position = texture(gPosition, fragUV).rgb;
    vec3 Normal = texture(gNormal, fragUV).rgb;

    if (Normal.xyz == vec3(0.0)){
        // return if sample the blank area in GBuffer
        discard;
    }
    
    vec3 albedo = texture(gAlbedo, fragUV).rgb;
    float metallic = texture(gMetallic, fragUV).r;
    float roughness = texture(gRoughness, fragUV).r;
    float ao = texture(gAo, fragUV).r;

    vec3 N = normalize(Normal);
    vec3 V = normalize(cameraPos - Position);

    // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)    
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);

    // reflectance equation
    vec3 Lo = vec3(0.0);
        
        // Shadow of Directional Light:
        vec4 fragPosLightSpace = lightSpaceMatrix * vec4(Position, 1.0);
        float shadowFactor = ShadowCalculation(fragPosLightSpace, shadow_map);

        // Directional Light
        vec3 L = normalize(-directionalLight.direction);
        vec3 H = normalize(V + L);
        vec3 radiance = directionalLight.color.xyz;  
        Lo += shadowFactor * radiance * BRDF(L, V, N, F0, radiance, metallic, roughness);  // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again

    for(int i = 0; i < point_lights_size; ++i) 
    {
        // calculate per-light radiance
        vec3 L = normalize(pointLights[i].position - Position);
        vec3 H = normalize(V + L);
        float distance = length(pointLights[i].position - Position);
        float attenuation = PointLightAttenuation(distance, pointLights[i].radius);
        vec3 radiance = pointLights[i].color.xyz * attenuation;  

        // Point Light Shadow:
        float pointShadowFactor = OmnidirectionalShadowCalculation(Position - pointLights[i].position, cube_shadow_maps[i], pointLights[i].radius);

        // add to outgoing radiance Lo
        Lo += pointShadowFactor * radiance * BRDF(L, V, N, F0, radiance, metallic, roughness);  // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
    }   
    
    // ambient lighting (note that the next IBL tutorial will replace 
    // this ambient lighting with environment lighting).
    vec3 ambient = 0.3 * albedo * ao;

    vec3 color = ambient + Lo;
    
    // gamma correct
    // color = GammaCorrection(color);

    FragColor = vec4(color, 1.0);
}