#version 330 core
layout (location = 0) out vec4 gPosition;
layout (location = 1) out vec4 gNormal;

layout (location = 2) out vec4 gDiffuse;
layout (location = 3) out vec4 gSpecular;

in VS_OUT{
    vec3 fragWorldPos;
    vec3 fragWorldNormal;
    vec2 fragUV;
} fs_in;

uniform sampler2D diffuse_map;
uniform sampler2D specular_map;
uniform vec3 diffuse_factor;
uniform vec3 specular_factor;

void main()
{    
    gPosition = vec4(fs_in.fragWorldPos, 1.0);
    gNormal = vec4(normalize(fs_in.fragWorldNormal), 1.0);
    gDiffuse = vec4(texture(diffuse_map, fs_in.fragUV).rgb * diffuse_factor, 1.0);
    gSpecular = vec4(texture(specular_map, fs_in.fragUV).rgb * specular_factor, 1.0);
}  
