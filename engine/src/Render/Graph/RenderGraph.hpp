#ifndef RenderGraph_hpp
#define RenderGraph_hpp

#include "Render/Graph/RenderGraphPassNode.hpp"

#include <array>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Pass 执行上下文的前置声明。
class RenderPassContext;
// RenderGraph 调试导出器的前置声明。
class RenderGraphDumper;

// 渲染图：声明 pass 的资源读写关系，编译依赖顺序，并负责执行与目标管理。
class RenderGraph {
public:
    using DisabledExecution = RenderGraphDisabledExecution;
    using ResourceAttachment = RenderGraphResourceAttachment;
    using RenderTargetKind = RenderGraphTargetKind;
    using ResourceBinding = RenderGraphResourceBinding;
    using ResourceDesc = RenderGraphResourceDesc;
    using PassNode = RenderGraphPassNode;

    ~RenderGraph();

    void reset();
    void setFrameSize(const Vec2& pixel_size);
    PassNode& addPass(const std::string& name, RenderPass::Type type, RenderPass* pass);
    void markOutput(const std::string& resource_name);
    void compile();
    void execute();

    RhiTexture* texture(const std::string& resource_name) const;
    RhiFrameBuffer* frameBuffer(const std::string& resource_name) const;
    RhiFrameBuffer* targetFrameBuffer(RenderPass::Type type, const std::string& target_name = RGTarget::Main) const;
    const std::vector<unsigned int>& cubeDepthTextures(RenderPass::Type type, const std::string& target_name = RGTarget::ShadowPointDepth) const;

private:
    friend class RenderPassContext;
    friend class RenderGraphDumper;

    // 编译后 resource 对应的生产者、最后修改者和底层目标信息。
    struct ResourceState {
        RenderPass::Type owner_pass;
        RenderPass::Type last_modifier;
        std::string target;
        ResourceBinding binding;
        ResourceDesc desc;
    };

    // RenderTargetState 的哈希 key：同一类 pass 可以持有多个命名目标。
    struct TargetKey {
        RenderPass::Type pass;
        std::string name;

        bool operator==(const TargetKey& other) const
        {
            return pass == other.pass && name == other.name;
        }
    };

    // TargetKey 在 unordered_map 中使用的哈希函数。
    struct TargetKeyHash {
        std::size_t operator()(const TargetKey& key) const
        {
            const auto pass_hash = std::hash<int>{}(static_cast<int>(key.pass));
            const auto name_hash = std::hash<std::string>{}(key.name);
            return pass_hash ^ (name_hash + 0x9e3779b9 + (pass_hash << 6) + (pass_hash >> 2));
        }
    };

    // framebuffer 创建所需的 attachment 描述集合。
    struct FrameBufferDesc {
        std::array<bool, 8> has_color{};
        std::array<ResourceDesc, 8> colors{};
        bool has_depth{ false };
        ResourceDesc depth;
        bool has_depth_stencil{ false };
        ResourceDesc depth_stencil;
    };

    // RenderGraph 持有的底层目标实例，包括普通 framebuffer 和 cube depth 目标。
    struct RenderTargetState {
        Vec2 size;
        RenderTargetKind kind{ RenderTargetKind::Texture };
        FrameBufferDesc desc;
        std::unique_ptr<RhiFrameBuffer> framebuffer;
        unsigned int cube_framebuffer{ 0 };
        int cube_edge{ 0 };
        int initial_cube_map_count{ 0 };
        std::vector<unsigned int> cube_maps;
    };

    PassNode* findNode(RenderPass::Type type);
    const PassNode* findNode(RenderPass::Type type) const;
    const RenderTargetState* targetState(RenderPass::Type type, const std::string& target_name) const;
    RenderTargetState* targetState(RenderPass::Type type, const std::string& target_name);
    static bool sameResourceDesc(const ResourceDesc& lhs, const ResourceDesc& rhs);
    static bool sameFrameBufferDesc(const FrameBufferDesc& lhs, const FrameBufferDesc& rhs);
    static bool isFrameBufferDescEmpty(const FrameBufferDesc& desc);
    void ensureTargets();
    void destroyCubeDepthTarget(RenderTargetState& state);
    void ensureCubeDepthTarget(RenderTargetState& state);
    void ensureCubeDepthTargetCapacity(RenderPass::Type type, const std::string& target_name, size_t count);
    void clearPassTargets(const PassNode& node);
    void clearTarget(const TargetKey& key, RenderTargetState& state);
    std::unique_ptr<RhiFrameBuffer> createFrameBuffer(const FrameBufferDesc& desc) const;
    std::unique_ptr<RhiFrameBuffer> createBackbufferFrameBuffer() const;
    void resolveResourceDependencies();
    void addResolvedDependency(PassNode& node, RenderPass::Type type);
    void resolveRead(PassNode& node, const std::string& resource_name);
    void resolveReadWrite(PassNode& node, const std::string& resource_name);
    void resolveWrite(PassNode& node, const PassNode::ResourceWrite& write);
    void visit(RenderPass::Type type, std::unordered_set<RenderPass::Type>& visiting, std::unordered_set<RenderPass::Type>& visited);

    std::shared_ptr<Rhi> m_rhi{ RenderSourceData::rhi };
    Vec2 m_frame_size{ DEFAULT_RENDER_RESOLUTION_X, DEFAULT_RENDER_RESOLUTION_Y };
    std::vector<PassNode> m_nodes;
    std::unordered_map<RenderPass::Type, size_t> m_node_indices;
    std::vector<std::string> m_resource_outputs;
    std::unordered_map<std::string, ResourceState> m_resources;
    std::unordered_map<TargetKey, RenderTargetState, TargetKeyHash> m_targets;
    std::vector<RenderPass::Type> m_compiled_order;
};

#endif // !RenderGraph_hpp
