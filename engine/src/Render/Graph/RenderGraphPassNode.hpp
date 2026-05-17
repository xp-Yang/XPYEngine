#ifndef RenderGraphPassNode_hpp
#define RenderGraphPassNode_hpp

#include "Render/Pass/RenderPass.hpp"

using RGResourceName = std::string;
using RGTargetName = std::string;

// 纹理资源名
namespace RGResource {
static inline RGResourceName PickingColor = "Picking.Color";
static inline RGResourceName PickingDepth = "Picking.Depth";
              
static inline RGResourceName ShadowDirectionalDepth = "Shadow.DirectionalDepth";
static inline RGResourceName ShadowDirectionalColor = "Shadow.DirectionalColor";
              
static inline RGResourceName GBufferPosition = "GBuffer.Position";
static inline RGResourceName GBufferNormal = "GBuffer.Normal";
static inline RGResourceName GBufferAlbedo = "GBuffer.Albedo";
static inline RGResourceName GBufferMetallic = "GBuffer.Metallic";
static inline RGResourceName GBufferRoughness = "GBuffer.Roughness";
static inline RGResourceName GBufferAO = "GBuffer.AO";
static inline RGResourceName GBufferDiffuse = "GBuffer.Diffuse";
static inline RGResourceName GBufferSpecular = "GBuffer.Specular";
static inline RGResourceName GBufferDepth = "GBuffer.Depth";
              
static inline RGResourceName SceneColor = "Scene.Color";
static inline RGResourceName SceneDepth = "Scene.Depth";
              
static inline RGResourceName BloomBrightColor = "Bloom.BrightColor";
static inline RGResourceName BloomPingPong1Color = "Bloom.PingPong1Color";
static inline RGResourceName BloomPingPong2Color = "Bloom.PingPong2Color";
              
static inline RGResourceName OutlineMaskColor = "Outline.MaskColor";
static inline RGResourceName OutlineMaskDepth = "Outline.MaskDepth";

static inline RGResourceName CheckerBoardColor = "CheckerBoard.Color";
static inline RGResourceName CheckerBoardDepth = "CheckerBoard.Depth";
              
static inline RGResourceName FinalColor = "Final.Color";
static inline RGResourceName FinalDepth = "Final.Depth";
}

// 渲染目标名，描述这些纹理资源挂载到哪个framebuffer上
namespace RGTarget {
// 普通framebuffer
static inline RGTargetName Main = "Main";
// 窗口默认framebuffer
static inline RGTargetName ScreenFrameBuffer = "ScreenFrameBuffer";
// BloomPass 的辅助 pingpong framebuffer
static inline RGTargetName BloomPingPong1 = "Bloom.PingPong1";
static inline RGTargetName BloomPingPong2 = "Bloom.PingPong2";
// OutlinePass 的辅助 mask framebuffer
static inline RGTargetName OutlineMask = "Outline.Mask";
// ShadowPass 的点光源 cubemap depth 的framebuffer
static inline RGTargetName ShadowPointDepth = "Shadow.PointDepth";
}

class RenderGraph;
class RenderPassContext;
class RenderGraphDumper;

// 关闭 pass 时的处理策略：跳过，或清理它声明的输出目标。
enum class RGDisabledExecution {
    Skip,
    Clear,
};

// TODO 放进rhi?
enum class RenderTargetType {
    FrameBuffer,
    ScreenFrameBuffer,
    CubeDepth,
};

// Pass 内部一个 framebuffer/cubemap/screenFrameBuffer 目标的声明。
struct RenderTargetDeclaration {
    RGTargetName name;
    RenderTargetType render_target_type{ RenderTargetType::FrameBuffer };
    int initial_cube_map_count{ 0 };
};

// Pass 写出的一个 resource
struct ResourceDeclaration {
    RGResourceName name;
    RGTargetName owner_target_name;
    RhiAttachmentDesc attachment_desc;
};

// Pass 声明节点：提供链式 API 描述一个 RenderPass 的资源读写和输出目标。
class RenderGraphPassNode {
public:
    RenderGraphPassNode(RenderPass::Type type, RenderPass* pass);

    // 声明renderTarget，可能是多个
    RenderGraphPassNode& target(const RGTargetName& target_name, RenderTargetType type, int initial_cube_map_count = 0);

    // 声明所需的资源，表明pass将从该resource采样
    RenderGraphPassNode& read(const RGResourceName& resource_name);

    // 声明续写的资源，表明pass将续写这个resource
    // 建立在旧图像内容的基础之上，如果不是，就write新resource
    // 但不能同时采样这个resource，否则会导致UB: feedback loop
    RenderGraphPassNode& modify(const RGResourceName& resource_name);

    // 声明produce的资源，一个资源只能有一个producer pass、只属于一个target，表明pass写出这个resource
    RenderGraphPassNode& color(const RGResourceName& resource_name, RhiTexture::Format format, int attachment_index = 0, int sample_count = 1, bool transient = true);
    RenderGraphPassNode& depth(const RGResourceName& resource_name, RhiTexture::Format format = RhiTexture::Format::DEPTH, int sample_count = 1, bool transient = true);
    RenderGraphPassNode& depthStencil(const RGResourceName& resource_name, RhiTexture::Format format = RhiTexture::Format::DEPTH24STENCIL8, int sample_count = 1, bool transient = true);

    // 注入回调
    RenderGraphPassNode& setEnabled(bool enabled);
    RenderGraphPassNode& setDisabledExecution(RGDisabledExecution execution);
    RenderGraphPassNode& setSetup(std::function<void(RenderPass&)> setup);

protected:
    void addResolvedDependency(RenderPass::Type type);

    RenderGraphPassNode& produce(const RGResourceName& resource_name, const RhiAttachmentDesc& desc);

private:
    friend class RenderGraph;
    friend class RenderPassContext;
    friend class RenderGraphDumper;

    RenderPass::Type m_type;
    RenderPass* m_pass{ nullptr };

    std::vector<RenderPass::Type> m_resolved_dependencies;

    std::vector<RGResourceName> m_reads;
    std::vector<RGResourceName> m_modifies;
    // 记录声明的写出的resource的targetName和Attachment信息
    // RenderGraph
    std::vector<ResourceDeclaration> m_resources;
    // 记录声明的RGTargetName和RenderTargetType信息，
    // RenderGraph的RenderGraphRenderTarget使用这些信息，
    // 用于创建framebuffer并记录framebuffer和targetName RenderPass的对应关系
    std::vector<RenderTargetDeclaration> m_targets;
    std::string m_active_target{ RGTarget::Main };

    bool m_enabled{ true };
    RGDisabledExecution m_disabled_execution{ RGDisabledExecution::Skip };
    std::function<void(RenderPass&)> m_setup;
};

#endif // !RenderGraphPassNode_hpp
