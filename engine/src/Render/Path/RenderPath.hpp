#ifndef RenderPipeline_hpp
#define RenderPipeline_hpp

#include "Base/Common.hpp"
#include "Render/Pass/RenderPass.hpp"

#include <string>
#include <vector>

// Interface class
class RenderPath {
public:
    virtual void render() = 0;
    virtual void prepareRenderSourceData(const std::shared_ptr<RenderSourceData>& render_source_data) {
        for (auto& pass : m_render_passes) {
            pass.second->prepareRenderSourceData(render_source_data);
        }
    }
    virtual unsigned int getPickingFBO() { return 0; }
    virtual RhiTexture* renderGraphTexture(const std::string& resource_name) { return nullptr; }
    virtual std::vector<std::string> renderGraphResourceNames() const { return {}; }
    virtual std::string renderGraphDebugDump() const { return {}; }
    virtual std::string renderGraphExecutionDump() const { return {}; }

    virtual void resizeRenderTargets(const Vec2& pixel_size) {}

protected:
    std::unordered_map<RenderPass::Type, std::unique_ptr<RenderPass>> m_render_passes;
};

#endif // !RenderPipeline_hpp
