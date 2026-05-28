#ifndef RenderPassContext_hpp
#define RenderPassContext_hpp

#include "Render/Graph/RenderGraph.hpp"
#include "Render/RenderBuiltinResources.hpp"
#include "Render/RenderFrameData.hpp"
#include "Render/RenderScene.hpp"

#include <string>
#include <vector>

// Pass 执行上下文：RenderGraph 在执行时注入，用于 pass 按资源名访问纹理和 FBO。
class RenderPassContext {
public:
    RenderPassContext(
        RenderGraph& graph,
        const RenderGraphPassNode& node,
        RenderScene& render_scene,
        RenderFrameData& frame_data,
        RenderBuiltinResources& builtin_resources);

    RenderScene& renderScene() const;
    RenderFrameData& frameData() const;
    RenderBuiltinResources& builtinResources() const;
    RhiTexture* texture(const std::string& resource_name) const;
    RhiFrameBuffer* frameBuffer(const std::string& resource_name) const;
    RhiFrameBuffer* frameBufferOfTarget(const std::string& target_name) const;
    RhiFrameBuffer* defaultFrameBuffer() const;

    std::vector<RhiTexture*> cubeShadowMaps() const;
    RhiFrameBuffer* cubeShadowFaceFrameBufferOf(size_t cube_index, int face) const;
    void ensureCubeShadowMapsCount(size_t count);

private:
    RenderGraph* m_graph{ nullptr };
    const RenderGraphPassNode* m_node{ nullptr };
    RenderScene* m_render_scene{ nullptr };
    RenderFrameData* m_frame_data{ nullptr };
    RenderBuiltinResources* m_builtin_resources{ nullptr };
};

#endif // !RenderPassContext_hpp
