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
	OneColorShader,
	SkyboxShader,
	NormalShader,
	WireframeShader,
	CheckerboardShader,
	RayTracingShader,
	CombineShader,
	CubeMapShader,
	ExtractBrightShader,
	GaussianBlur,
	OutlineShader,
	InstancingShader,
	BillBoardShader,
	FXAAShader,
};

struct Shader {
	Shader(const std::string& vs_filepath, const std::string& fs_filepath);
	Shader(const std::string& vs_filepath, const std::string& fs_filepath, const std::string& gs_filepath);

	std::string vsCode;
	std::string fsCode;
	std::string gsCode;
};

#endif

