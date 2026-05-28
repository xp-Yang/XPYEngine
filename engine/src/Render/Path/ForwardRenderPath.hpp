#ifndef ForwardRenderPath_hpp
#define ForwardRenderPath_hpp

#include "RenderPath.hpp"

class ForwardRenderPath : public RenderPath {
public:
    ForwardRenderPath(RenderSystem* render_system);
    void render(RenderScene& render_scene, RenderFrameData& frame_data, RenderBuiltinResources& builtin_resources) override;
    void resizeRenderTargets(const Vec2& pixel_size) override;
    RhiTexture* renderGraphTextureOf(const std::string& resource_name) override;
    bool readRenderGraphPixelRGBAOf(const std::string& resource_name, int x, int y, unsigned char out_rgba[4]) override;
    std::vector<std::string> renderGraphResourceNames() const override;
    std::vector<RenderGraphResourceDebugInfo> renderGraphResourceDebugInfos() const override;
    std::string renderGraphDebugDump() const override;
    std::string renderGraphExecutionDump() const override;
};

#endif // !ForwardRenderPath_hpp
