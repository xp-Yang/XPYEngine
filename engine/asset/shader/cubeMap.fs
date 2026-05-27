#version 330 core

in VS_OUT {
    vec3 fragWorldPos;
    vec3 fragWorldNormal;
    vec2 fragUV;
} fs_in;

uniform vec3 lightPos;
uniform float far_plane;

void main()
{
    float lightDistance = length(fs_in.fragWorldPos - lightPos);

    lightDistance = lightDistance / far_plane;

    gl_FragDepth = lightDistance;
}