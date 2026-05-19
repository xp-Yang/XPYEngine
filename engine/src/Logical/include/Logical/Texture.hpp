#ifndef Texture_hpp
#define Texture_hpp

#include <string>
#include <vector>
#include <array>

enum class TextureType {
	None,
	Diffuse,
	Specular,
	Normal,
	Height,
	Albedo,
	Metallic,
	Roughness,
	AO,
	Custom,
};

struct Texture {
    Texture() = default;
	Texture(TextureType type, const std::string& filepath, bool gamma);

	TextureType texture_type{ TextureType::None };
	std::string texture_filepath;
	int width{ 0 };
	int height{ 0 };
	int channel_count{ 0 };
    unsigned char* data{ nullptr };
	bool gamma{ false };

	void freeData();
};

struct CubeTexture {
    CubeTexture() = default;
	CubeTexture(
		const std::string& right_filepath,
		const std::string& left_filepath,
		const std::string& top_filepath,
		const std::string& bottom_filepath,
		const std::string& front_filepath,
		const std::string& back_filepath);

	std::array<std::string, 6> cube_texture_filepath;
	int width{ 0 };
	int height{ 0 };
	int channel_count{ 0 };
	std::array<unsigned char*, 6> datas;
	bool gamma{ false };

	void freeData();
};

#endif
