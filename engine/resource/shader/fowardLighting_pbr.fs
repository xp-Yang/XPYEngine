#version 330 core

#include "common.h"
#include "BRDF.h"

in VS_OUT {
    vec3 fragWorldPos;          //世界坐标
    vec3 fragWorldNormal;       //局部坐标
    vec2 fragUV;
} fs_in;

uniform sampler2D albedo_map;
uniform sampler2D metallic_map;
uniform sampler2D roughness_map;
uniform sampler2D ao_map;

uniform vec3 cameraPos;

out vec4 FragColor;

void main()
{		
    vec3 N = normalize(fs_in.fragWorldNormal);
    vec3 V = normalize(cameraPos - fs_in.fragWorldPos);

    vec3 albedo = texture(albedo_map, fs_in.fragUV).rgb;
    float metallic = texture(metallic_map, fs_in.fragUV).r;
    float roughness = texture(roughness_map, fs_in.fragUV).r;
    float ao = texture(ao_map, fs_in.fragUV).r;

    // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)    
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);

    // reflectance equation
    vec3 Lo = vec3(0.0);
		
        // Shadow of Directional Light:
        vec4 fragPosLightSpace = lightSpaceMatrix * vec4(fs_in.fragWorldPos, 1.0);
        float shadowFactor = ShadowCalculation(fragPosLightSpace, shadow_map);

		// Directional Light
        vec3 L = normalize(-directionalLight.direction);
        vec3 H = normalize(V + L);
        vec3 radiance = directionalLight.color.xyz;  
        Lo += shadowFactor * radiance * BRDF(L, V, N, F0, radiance, metallic, roughness);  // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again

    for(int i = 0; i < point_lights_size; ++i) 
    {
        // calculate per-light radiance
        vec3 L = normalize(pointLights[i].position - fs_in.fragWorldPos);
        vec3 H = normalize(V + L);
        float distance = length(pointLights[i].position - fs_in.fragWorldPos);
        float attenuation = PointLightAttenuation(distance, pointLights[i].radius);
        vec3 radiance = pointLights[i].color.xyz * attenuation;  

        // add to outgoing radiance Lo
        Lo += radiance * BRDF(L, V, N, F0, radiance, metallic, roughness);  // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
    }   
    
    // ambient lighting (note that the next IBL tutorial will replace 
    // this ambient lighting with environment lighting).
    vec3 ambient = 0.3 * albedo * ao;

    vec3 color = ambient + Lo;

    // gamma correct
    color = GammaCorrection(color);

    FragColor = vec4(color, 1.0);
}
