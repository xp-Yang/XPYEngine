#ifndef RenderGraph_hpp
#define RenderGraph_hpp

#include "Render/Graph/RenderGraphPassNode.hpp"

class RenderPassContext;
class RenderGraphDumper;

// RenderGraphRenderTarget key
struct TargetKey {
    RenderPass::Type pass;
    RGTargetName name;

    bool operator==(const TargetKey& other) const
    {
        return pass == other.pass && name == other.name;
    }
};
struct TargetKeyHasher {
    std::size_t operator()(const TargetKey& key) const
    {
        const auto pass_hash = std::hash<int>{}(static_cast<int>(key.pass));
        const auto name_hash = std::hash<std::string>{}(key.name);
        return pass_hash ^ (name_hash + 0x9e3779b9 + (pass_hash << 6) + (pass_hash >> 2));
    }
};
// 同一类 pass 可以持有多个target, 包括普通 framebuffer 和 cube depth 等。
struct RenderGraphRenderTarget {
    RenderTargetDeclaration target_decl;

    RhiFrameBufferDesc framebuffer_desc;
    std::unique_ptr<RhiFrameBuffer> framebuffer;

    std::vector<std::array<std::unique_ptr<RhiFrameBuffer>, 6>> cube_shadow_framebuffers;
};

// 声明 pass 的资源读写关系，编译依赖顺序，并负责执行与目标管理。
class RenderGraph {
public:
    ~RenderGraph();

    void reset();
    void setFrameSize(const Vec2& pixel_size);
    RenderGraphPassNode& addPass(RenderPass::Type type, RenderPass* pass);
    void markOutput(const RGResourceName& resource_name);
    void compile();
    void execute(RenderSourceData& render_source_data);

    // 查询接口
    RhiTexture* textureOf(const RGResourceName& resource_name) const;
    RhiFrameBuffer* frameBufferOf(const RGResourceName& resource_name) const;
    bool readPixelRGBAOf(const RGResourceName& resource_name, int x, int y, unsigned char out_rgba[4]) const;
    std::vector<RhiTexture*> cubeShadowMapsOf(RenderPass::Type type) const;

protected:
    // 内部查询接口
    const RenderGraphRenderTarget* findRenderTarget(RenderPass::Type type, const RGTargetName& target_name) const;
    RenderGraphRenderTarget* findRenderTarget(RenderPass::Type type, const RGTargetName& target_name);

    void ensureRenderTargets();
    void clearPassTargets(const RenderGraphPassNode& node);

    std::unique_ptr<RhiFrameBuffer> createFrameBuffer(const RhiFrameBufferDesc& desc) const;
    std::unique_ptr<RhiFrameBuffer> createDefaultFrameBuffer() const;

    void ensureCubeShadowMapsCount(RenderPass::Type type, size_t count);
    void appendCubeShadowMap(RenderGraphRenderTarget& target);
    void destroyCubeShadowFrameBuffer(RenderGraphRenderTarget& target);
    RhiFrameBuffer* cubeShadowFaceFrameBufferOf(RenderPass::Type type, size_t cube_index, int face) const;

    void resolveResourceDependencies();
    void resolveReads(RenderGraphPassNode& node);
    void resolveModifies(RenderGraphPassNode& node);
    void resolveResources(RenderGraphPassNode& node);
    void visit(RenderPass::Type type, std::unordered_set<RenderPass::Type>& visiting, std::unordered_set<RenderPass::Type>& visited);

private:
    friend class RenderPassContext;
    friend class RenderGraphDumper;

    Rhi* m_rhi{ Rhi::get() };
    Vec2 m_frame_size{ DEFAULT_RENDER_RESOLUTION_X, DEFAULT_RENDER_RESOLUTION_Y };
    std::vector<RenderGraphPassNode> m_nodes;
    std::vector<RenderGraphPassNode*> m_ordered_nodes;
    std::vector<RGResourceName> m_outputs;
    std::unordered_map<RGResourceName, RenderGraphResource> m_resources;
    std::unordered_map<TargetKey, RenderGraphRenderTarget, TargetKeyHasher> m_render_targets;
};

#endif // !RenderGraph_hpp
