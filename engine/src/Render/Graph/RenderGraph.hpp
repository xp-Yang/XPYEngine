#ifndef RenderGraph_hpp
#define RenderGraph_hpp

#include "Render/Graph/RenderGraphPassNode.hpp"

class RenderPassContext;
class RenderGraphDumper;

// RenderGraphRenderTarget 的哈希 key：同一类 pass 可以持有多个命名目标。
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
// RenderGraph 持有的底层目标实例，包括普通 framebuffer 和 cube depth 目标。
struct RenderGraphRenderTarget {
    RenderPass::Type owner_pass;

    RenderTargetDeclaration target_decl;

    FrameBufferDesc framebuffer_desc;
    std::unique_ptr<RhiFrameBuffer> framebuffer;
    // TODO rhi支持
    unsigned int cube_framebuffer{ 0 };
    int cube_edge{ 0 };
    std::vector<unsigned int> cube_maps;
};

// 编译后 resource 对应的生产者、最后修改者和底层目标信息。
struct RenderGraphResource {
    RenderPass::Type owner_pass;
    RenderPass::Type last_modifier_pass;

    ResourceDeclaration resource_decl;
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
    const std::vector<unsigned int>& cubeDepthTextures(RenderPass::Type type) const;

private:
    friend class RenderPassContext;
    friend class RenderGraphDumper;

    // 内部查询接口
    const RenderGraphRenderTarget* findRenderTarget(RenderPass::Type type, const RGTargetName& target_name) const;
    RenderGraphRenderTarget* findRenderTarget(RenderPass::Type type, const RGTargetName& target_name);

    // 在执行前确保每个 pass 需要的 render target 已经存在、尺寸正确、attachment 布局正确。
    void ensureRenderTargets();
    void destroyCubeDepthTarget(RenderGraphRenderTarget& target);
    void ensureCubeDepthTarget(RenderGraphRenderTarget& target);
    void ensureCubeDepthTargetCapacity(RenderPass::Type type, size_t count);
    void clearPassTargets(const RenderGraphPassNode& node);
    std::unique_ptr<RhiFrameBuffer> createFrameBuffer(const FrameBufferDesc& desc) const;
    std::unique_ptr<RhiFrameBuffer> createDefaultFrameBuffer() const;
    void resolveResourceDependencies();
    void resolveReads(RenderGraphPassNode& node);
    void resolveModifies(RenderGraphPassNode& node);
    void resolveResources(RenderGraphPassNode& node);
    void visit(RenderPass::Type type, std::unordered_set<RenderPass::Type>& visiting, std::unordered_set<RenderPass::Type>& visited);

    std::shared_ptr<Rhi> m_rhi{ RenderSourceData::rhi };
    Vec2 m_frame_size{ DEFAULT_RENDER_RESOLUTION_X, DEFAULT_RENDER_RESOLUTION_Y };
    std::vector<RenderGraphPassNode> m_nodes;
    std::vector<RenderGraphPassNode*> m_ordered_nodes;
    std::vector<RGResourceName> m_outputs;
    std::unordered_map<RGResourceName, RenderGraphResource> m_resources;
    std::unordered_map<TargetKey, RenderGraphRenderTarget, TargetKeyHasher> m_render_targets;
};

#endif // !RenderGraph_hpp
