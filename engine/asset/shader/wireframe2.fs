#version 330 core

in vec3 barycentric_coords;

out vec4 outColor;

float edge(float pixelWidth) {
    float min_t = min(min(barycentric_coords.x, barycentric_coords.y), barycentric_coords.z);
    return smoothstep(-fwidth(min_t) * pixelWidth, 0.0, min_t) -
        smoothstep(0.0, fwidth(min_t) * pixelWidth, min_t);
}

void main() {
    float t = edge(2.0);
    if (t <= 0)
        discard;
    else
        outColor = t * vec4(0.18, 0.18, 0.18, 1.0);
}