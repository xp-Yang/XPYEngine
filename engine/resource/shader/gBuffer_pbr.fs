#version 330 core
layout (location = 0) out vec4 gPosition;
layout (location = 1) out vec4 gNormal;

layout (location = 2) out vec4 gAlbedo;
layout (location = 3) out vec4 gMetallic;
layout (location = 4) out vec4 gRoughness;
layout (location = 5) out vec4 gAo;

in VS_OUT{
    vec3 fragWorldPos;
    vec3 fragWorldNormal;
    vec2 fragUV;
} fs_in;

uniform vec4 base_color_factor;
uniform float metallic_factor;
uniform float roughness_factor;
uniform float ao_factor;

uniform sampler2D albedo_map;
uniform sampler2D metallic_map;
uniform sampler2D roughness_map;
uniform sampler2D ao_map;

void main()
{    
    gPosition = vec4(fs_in.fragWorldPos, 1.0);
    gNormal = vec4(normalize(fs_in.fragWorldNormal), 1.0);

    vec4 albedo = texture(albedo_map, fs_in.fragUV) * base_color_factor;
    float metallic = texture(metallic_map, fs_in.fragUV).r * metallic_factor;
    float roughness = texture(roughness_map, fs_in.fragUV).r * roughness_factor;
    float ao = texture(ao_map, fs_in.fragUV).r * ao_factor;

    gAlbedo = vec4(albedo.rgb, albedo.a);
    gMetallic = vec4(vec3(metallic), 1.0);
    gRoughness = vec4(vec3(roughness), 1.0);
    gAo = vec4(vec3(ao), 1.0);
}  
