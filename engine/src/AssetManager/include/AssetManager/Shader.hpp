#ifndef Shader_hpp
#define Shader_hpp

#include "Base/Common.hpp"

enum class ShaderType {
	None,
	PristineGridShader,
	GBufferPhongShader,
	DeferredLightingPhongShader,
	GBufferShader,
	DeferredLightingShader,
	BlinnPhongShader,
	PBRShader,
	SingleColorShader,
	SkyboxShader,
	NormalShader,
	WireframeShader,
	CheckerboardShader,
	RayTracingShader,
	CubeMapShader,
	ExtractBrightShader,
	GaussianBlur,
	BloomShader,
	OutlineShader,
	InstancingShader,
	BillBoardShader,
	FXAAShader,
	DebugTexturePreviewShader,
	TransparentShader,
};

struct Shader {
	Shader(const std::string& vs_filepath, const std::string& fs_filepath);
	Shader(const std::string& vs_filepath, const std::string& fs_filepath, const std::string& gs_filepath);

	static Shader create(const ShaderType& type);
	static const Shader& get(const ShaderType& type);

	std::string vsCode;
	std::string fsCode;
	std::string gsCode;
};

#endif

