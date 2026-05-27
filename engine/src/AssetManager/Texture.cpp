#include "AssetManager/Texture.hpp"
#include "stb_image.h"
#include <cassert>

Texture::Texture(TextureType type, const std::string& filepath, bool gamma)
	: texture_type(type)
	, texture_filepath(filepath)
	, gamma(gamma)
{
    data = stbi_load(filepath.c_str(), &width, &height, &channel_count, 0);

    if (!data)
    {
        assert(false);
        stbi_image_free(data);
    }
}

void Texture::freeData()
{
    if (data)
        stbi_image_free(data);
}

CubeTexture::CubeTexture(
        const std::string& right_filepath,
        const std::string& left_filepath,
        const std::string& top_filepath,
        const std::string& bottom_filepath,
        const std::string& front_filepath,
        const std::string& back_filepath)
{
    cube_texture_filepath[0] = right_filepath;
    cube_texture_filepath[1] = left_filepath;
    cube_texture_filepath[2] = top_filepath;
    cube_texture_filepath[3] = bottom_filepath;
    cube_texture_filepath[4] = front_filepath;
    cube_texture_filepath[5] = back_filepath;

    for (int i = 0; i < cube_texture_filepath.size(); i++) {
        datas[i] = stbi_load(cube_texture_filepath[i].c_str(), &width, &height, &channel_count, 0);
        if (!datas[i])
        {
            assert(false);
            stbi_image_free(datas[i]);
        }
    }
}

void CubeTexture::freeData()
{
    for (int i = 0; i < datas.size(); i++) {
        if (datas[i])
            stbi_image_free(datas[i]);
    }
}
