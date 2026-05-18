#ifndef RenderPipeline_hpp
#define RenderPipeline_hpp

#include "Base/Common.hpp"
#include "Render/Pass/RenderPass.hpp"
#include "Render/Graph/RenderGraph.hpp"
#include "Render/Graph/RenderGraphDebugInfo.hpp"

#include <string>
#include <vector>

// Interface class
class RenderSystem;
class RenderPath {
public:
    virtual void render(RenderSourceData& render_source_data) = 0;

    virtual RhiTexture* renderGraphTextureOf(const std::string& resource_name) { return nullptr; }
    virtual bool readRenderGraphPixelRGBAOf(const std::string& resource_name, int x, int y, unsigned char out_rgba[4]) { return false; }

    virtual std::vector<std::string> renderGraphResourceNames() const { return {}; }
    virtual std::vector<RenderGraphResourceDebugInfo> renderGraphResourceDebugInfos() const { return {}; }
    virtual std::string renderGraphDebugDump() const { return {}; }
    virtual std::string renderGraphExecutionDump() const { return {}; }

    virtual void resizeRenderTargets(const Vec2& pixel_size) {}

protected:
    std::unordered_map<RenderPass::Type, std::unique_ptr<RenderPass>> m_render_passes;
    RenderGraph m_render_graph;
    RenderSystem* ref_render_system{ nullptr };
};

#endif // !RenderPipeline_hpp
