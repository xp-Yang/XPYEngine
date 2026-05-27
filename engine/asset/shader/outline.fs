#version 330 core

in vec2 fragUV;

uniform sampler2D objMap;
uniform sampler2D objDepthMap;

out vec4 outColor;

void main()
{
    const int RobertsCrossX[4] = int[4](
        1, 0,
        0, -1
    );

    const int RobertsCrossY[4] = int[4](
        0, 1,
        -1, 0
    );

    vec2 tex_offset = 1.0 / textureSize(objMap, 0);
    vec3 topLeftColor = texture(objMap, fragUV).rgb;
    vec3 topRightColor = texture(objMap, fragUV + vec2(tex_offset.x, 0.0)).rgb;
    vec3 bottomLeftColor = texture(objMap, fragUV + vec2(0.0, -tex_offset.y)).rgb;
    vec3 bottomRightColor = texture(objMap, fragUV + vec2(tex_offset.x, -tex_offset.y)).rgb;

    vec3 horizontal = topLeftColor * RobertsCrossX[0]; // top left (factor +1)
    horizontal += bottomRightColor * RobertsCrossX[3]; // bottom right (factor -1)

    vec3 vertical = bottomLeftColor * RobertsCrossY[2]; // bottom left (factor -1)
    vertical += topRightColor * RobertsCrossY[1]; // top right (factor +1)

    float edge = sqrt(dot(horizontal, horizontal) + dot(vertical, vertical));

    if (fwidth(edge) == 0)
        discard;
    
    outColor = vec4(vec3(1.0), 1.0);
    gl_FragDepth = texture(objDepthMap, fragUV).r;
}