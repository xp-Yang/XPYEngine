#version 330 core

in vec2 fragUV;
out float FragColor;

uniform sampler2D ssaoInput;
uniform vec2 texelSize;

// 4x4 邻域均值模糊，去除程序化噪声造成的颗粒。
void main()
{
    float result = 0.0;
    for (int x = -2; x < 2; ++x)
    {
        for (int y = -2; y < 2; ++y)
        {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(ssaoInput, fragUV + offset).r;
        }
    }
    FragColor = result / 16.0;
}
