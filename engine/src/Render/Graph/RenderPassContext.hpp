#ifndef RenderPassContext_hpp
#define RenderPassContext_hpp

#include "Render/Graph/RenderGraph.hpp"

#include <string>
#include <unordered_map>
#include <vector>

// Pass 执行上下文：RenderGraph 在执行时注入，用于 pass 按资源名或语义 slot 访问纹理和 FBO。
class RenderPassContext {
public:
    RenderPassContext(RenderGraph& graph, const RenderGraph::PassNode& node);

    RhiTexture* texture(const std::string& resource_name) const;
    RhiFrameBuffer* frameBuffer(const std::string& resource_name) const;
    RhiFrameBuffer* targetFrameBuffer(const std::string& target_name = RGTarget::Main) const;
    RhiFrameBuffer* defaultFrameBuffer() const;
    const std::vector<unsigned int>& cubeDepthTextures(const std::string& target_name = RGTarget::ShadowPointDepth) const;
    unsigned int cubeDepthFrameBuffer(const std::string& target_name = RGTarget::ShadowPointDepth) const;
    int cubeDepthEdge(const std::string& target_name = RGTarget::ShadowPointDepth) const;
    void ensureCubeDepthTextureCount(const std::string& target_name, size_t count);

    RhiFrameBuffer* readFrameBuffer(const std::string& slot_name) const;
    RhiTexture* readTexture(const std::string& slot_name) const;
    RhiFrameBuffer* readWriteFrameBuffer(const std::string& slot_name) const;

private:
    const std::string* resourceNameForSlot(const std::unordered_map<std::string, std::string>& slot_bindings, const std::string& slot_name) const;

    RenderGraph* m_graph{ nullptr };
    const RenderGraph::PassNode* m_node{ nullptr };
};

#endif // !RenderPassContext_hpp
