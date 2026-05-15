#ifndef RenderGraphPassNode_hpp
#define RenderGraphPassNode_hpp

#include "Render/Pass/RenderPass.hpp"

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace RGResource {
static inline constexpr const char* PickingColor = "Picking.Color";
static inline constexpr const char* PickingDepth = "Picking.Depth";

static inline constexpr const char* ShadowDirectionalDepth = "Shadow.DirectionalDepth";
static inline constexpr const char* ShadowDirectionalColor = "Shadow.DirectionalColor";

static inline constexpr const char* GBufferPosition = "GBuffer.Position";
static inline constexpr const char* GBufferNormal = "GBuffer.Normal";
static inline constexpr const char* GBufferAlbedo = "GBuffer.Albedo";
static inline constexpr const char* GBufferMetallic = "GBuffer.Metallic";
static inline constexpr const char* GBufferRoughness = "GBuffer.Roughness";
static inline constexpr const char* GBufferAO = "GBuffer.AO";
static inline constexpr const char* GBufferDiffuse = "GBuffer.Diffuse";
static inline constexpr const char* GBufferSpecular = "GBuffer.Specular";
static inline constexpr const char* GBufferDepth = "GBuffer.Depth";

static inline constexpr const char* SceneColor = "Scene.Color";
static inline constexpr const char* SceneDepth = "Scene.Depth";

static inline constexpr const char* CheckerBoardColor = "CheckerBoard.Color";
static inline constexpr const char* CheckerBoardDepth = "CheckerBoard.Depth";

static inline constexpr const char* BloomColor = "Bloom.Color";
static inline constexpr const char* BloomPingPongColor = "Bloom.PingPongColor";

static inline constexpr const char* OutlineMaskColor = "Outline.MaskColor";
static inline constexpr const char* OutlineMaskDepth = "Outline.MaskDepth";

static inline constexpr const char* FinalColor = "Final.Color";
static inline constexpr const char* FinalDepth = "Final.Depth";
}

namespace RGTarget {
static inline constexpr const char* Main = "Main";
static inline constexpr const char* Backbuffer = "Backbuffer";
static inline constexpr const char* BloomPingPong = "Bloom.PingPong";
static inline constexpr const char* OutlineMask = "Outline.Mask";
static inline constexpr const char* ShadowPointDepth = "Shadow.PointDepth";
}

namespace RGSlot {
static inline constexpr const char* Source = "Source";
static inline constexpr const char* Target = "Target";
static inline constexpr const char* GBuffer = "GBuffer";
static inline constexpr const char* Bloom = "Bloom";
}

// 渲染图本体的前置声明。
class RenderGraph;
// Pass 执行上下文的前置声明。
class RenderPassContext;
// RenderGraph 调试导出器的前置声明。
class RenderGraphDumper;

// 关闭 pass 时的处理策略：跳过，或清理它声明的输出目标。
enum class RenderGraphDisabledExecution {
    Skip,
    Clear,
};

// graph resource 在 framebuffer 上绑定到的 attachment 类型。
enum class RenderGraphResourceAttachment {
    Color,
    Depth,
    DepthStencil,
};

// RenderGraph 管理的渲染目标类型。
enum class RenderGraphTargetKind {
    Texture,
    Backbuffer,
    CubeDepth,
};

// 单个 resource 在 framebuffer 中的 attachment 绑定信息。
struct RenderGraphResourceBinding {
    static RenderGraphResourceBinding color(int attachment_index = 0);
    static RenderGraphResourceBinding depth();
    static RenderGraphResourceBinding depthStencil();

    RenderGraphResourceAttachment attachment{ RenderGraphResourceAttachment::Color };
    int color_attachment{ 0 };
};

// 单个 resource 的纹理格式、采样数和生命周期描述。
struct RenderGraphResourceDesc {
    static RenderGraphResourceDesc texture(RhiTexture::Format format, int sample_count = 1, bool transient = true);

    RhiTexture::Format format{ RhiTexture::Format::UnknownFormat };
    int sample_count{ 1 };
    bool transient{ true };
};

// Pass 声明节点：提供链式 API 描述一个 RenderPass 的资源读写和输出目标。
class RenderGraphPassNode {
public:
    // Pass 内部一个 framebuffer/cubemap/backbuffer 目标的声明。
    struct TargetDeclaration {
        std::string name;
        RenderGraphTargetKind kind{ RenderGraphTargetKind::Texture };
        int initial_cube_map_count{ 0 };
    };

    // Pass 写出的一个 graph resource 以及它所在的目标和 attachment。
    struct ResourceWrite {
        std::string name;
        std::string target;
        RenderGraphResourceBinding binding;
        RenderGraphResourceDesc desc;
    };

    RenderGraphPassNode(std::string name, RenderPass::Type type, RenderPass* pass);

    RenderGraphPassNode& read(const std::string& resource_name);
    RenderGraphPassNode& readAs(const std::string& slot_name, const std::string& resource_name);
    RenderGraphPassNode& readWrite(const std::string& resource_name);
    RenderGraphPassNode& readWriteAs(const std::string& slot_name, const std::string& resource_name);
    RenderGraphPassNode& mainTarget();
    RenderGraphPassNode& target(const std::string& target_name, RenderGraphTargetKind kind = RenderGraphTargetKind::Texture);
    RenderGraphPassNode& backbuffer(const std::string& target_name = RGTarget::Backbuffer);
    RenderGraphPassNode& cubeDepthTarget(const std::string& target_name = RGTarget::ShadowPointDepth, int initial_cube_map_count = 8);
    RenderGraphPassNode& color(const std::string& resource_name, RhiTexture::Format format, int attachment_index = 0, int sample_count = 1, bool transient = true);
    RenderGraphPassNode& depth(const std::string& resource_name, RhiTexture::Format format = RhiTexture::Format::DEPTH, int sample_count = 1, bool transient = true);
    RenderGraphPassNode& depthStencil(const std::string& resource_name, RhiTexture::Format format = RhiTexture::Format::DEPTH24STENCIL8, int sample_count = 1, bool transient = true);
    RenderGraphPassNode& setEnabled(bool enabled);
    RenderGraphPassNode& setDisabledExecution(RenderGraphDisabledExecution execution);
    RenderGraphPassNode& setSetup(std::function<void(RenderPass&)> setup);

private:
    friend class RenderGraph;
    friend class RenderPassContext;
    friend class RenderGraphDumper;

    void declareTarget(const std::string& target_name, RenderGraphTargetKind kind, int initial_cube_map_count = 0);
    RenderGraphPassNode& writeTo(const std::string& target_name, const std::string& resource_name, RenderGraphResourceBinding binding, RenderGraphResourceDesc desc);
    void bindSlot(std::unordered_map<std::string, std::string>& slot_bindings, const std::string& slot_name, const std::string& resource_name);

    std::string m_name;
    RenderPass::Type m_type;
    RenderPass* m_pass{ nullptr };
    std::string m_active_target{ RGTarget::Main };
    bool m_enabled{ true };
    RenderGraphDisabledExecution m_disabled_execution{ RenderGraphDisabledExecution::Skip };
    std::vector<RenderPass::Type> m_resolved_dependencies;
    std::vector<std::string> m_reads;
    std::vector<std::string> m_read_writes;
    std::unordered_map<std::string, std::string> m_read_slots;
    std::unordered_map<std::string, std::string> m_read_write_slots;
    std::vector<TargetDeclaration> m_targets;
    std::vector<ResourceWrite> m_writes;
    std::function<void(RenderPass&)> m_setup;
};

#endif // !RenderGraphPassNode_hpp
