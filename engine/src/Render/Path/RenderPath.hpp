#ifndef RenderPipeline_hpp
#define RenderPipeline_hpp

#include "Base/Common.hpp"
#include "Render/Pass/RenderPass.hpp"

// Interface class
class RenderPath {
public:
    virtual void render() = 0;
    virtual void prepareRenderSourceData(const std::shared_ptr<RenderSourceData>& render_source_data) {
        for (auto& pass : m_render_passes) {
            pass.second->prepareRenderSourceData(render_source_data);
        }
    }
    virtual unsigned int getPickingFBO() { 
        if (m_render_passes.find(RenderPass::Type::Picking) != m_render_passes.end())
            return m_render_passes[RenderPass::Type::Picking]->getFrameBuffer()->id();
        return 0;
    }
    virtual RhiTexture* renderPassTexture(RenderPass::Type render_pass_type) {
        if (m_render_passes.find(render_pass_type) != m_render_passes.end()) {
            if (render_pass_type == RenderPass::Type::Shadow)
                return m_render_passes[render_pass_type]->getFrameBuffer()->depthAttachment()->texture();
            return m_render_passes[render_pass_type]->getFrameBuffer()->colorAttachmentAt(0)->texture();
        }
        return nullptr;
    }

    virtual void rebuildFramebuffers(const Vec2& pixel_size) {
        for (auto& pass : m_render_passes)
            pass.second->rebuildFramebuffers(pixel_size);
    }

protected:
    std::unordered_map<RenderPass::Type, std::unique_ptr<RenderPass>> m_render_passes;
};

#endif // !RenderPipeline_hpp
