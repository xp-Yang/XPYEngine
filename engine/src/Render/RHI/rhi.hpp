#pragma once

#include <Base/Common.hpp>

using GL_HANDLE = unsigned int;

class RhiResource;
class RhiBuffer;
class RhiTexture;
class RhiAttachment;
class RhiFrameBuffer;
class RhiCommandBuffer;
class RhiGraphicsPipeline;
class RhiShaderResourceBindings;
class Window;
class RhiSwapChain;
class RhiImpl;

class RhiResource
{
public:
    enum Type
    {
        Buffer,
        Texture,
        Sampler,
        RenderBuffer,
        RenderPassDescriptor,
        SwapChainRenderTarget,
        TextureRenderTarget,
        ShaderResourceBindings,
        GraphicsPipeline,
        SwapChain,
        ComputePipeline,
        CommandBuffer
    };
};

class RhiBuffer
{
public:
    enum Type
    {
        Immutable,
        Static,
        Dynamic
    };

    enum UsageFlag
    {
        VertexBuffer = 1 << 0,
        IndexBuffer = 1 << 1,
        UniformBuffer = 1 << 2,
        StorageBuffer = 1 << 3
    };

    Type type() const { return m_type; }
    void setType(Type t) { m_type = t; }

    UsageFlag usage() const { return m_usage; }
    void setUsage(UsageFlag u) { m_usage = u; }

    int size() const { return m_size; }
    void setSize(int sz) { m_size = sz; }

    GL_HANDLE id() const { return m_id; }

    virtual bool create() = 0;
    virtual void update(void* data, int size, int offset = 0) = 0;

protected:
    RhiBuffer(Type type_, UsageFlag usage_, void *data, int size_);
    Type m_type;
    UsageFlag m_usage;
    void *m_data{nullptr};
    int m_size;
    GL_HANDLE m_id{0};
};

class RhiTexture
{
public:
    virtual ~RhiTexture() = default;

    enum Flag
    {
        RenderTarget = 1 << 0,
        CubeMap = 1 << 2,
        MipMapped = 1 << 3,
        sRGB = 1 << 4,
        //UsedAsTransferSource = 1 << 5,
        //UsedWithGenerateMips = 1 << 6,
        //UsedWithLoadStore = 1 << 7,
        //UsedAsCompressedAtlas = 1 << 8,
        //ExternalOES = 1 << 9,
        //TextureRectangleGL = 1 << 11,
        TextureArray = 1 << 12,
        OneDimensional = 1 << 13,
        ThreeDimensional = 1 << 14,
    };

    enum Format
    {
        UnknownFormat,

        R8,

        RGB8,
        RGB16F,

        RGBA8,
        RGBA16F,

        DEPTH24STENCIL8,
        DEPTH,
    };

    Format format() const { return m_format; }
    void setFormat(Format fmt) { m_format = fmt; }

    Vec2 pixelSize() const { return m_pixelSize; }
    void setPixelSize(const Vec2 &sz) { m_pixelSize = sz; }

    Flag flags() const { return m_flags; }
    void setFlags(Flag f) { m_flags = f; }

    int sampleCount() const { return m_sampleCount; }
    void setSampleCount(int s) { m_sampleCount = s; }

    GL_HANDLE id() const { return m_id; }

    virtual bool create() = 0; // true generate and bind texture
    virtual void destroy(); // release GL name; safe to call multiple times

protected:
    RhiTexture(Format format_, const Vec2 &pixelSize_, int sampleCount_, Flag flags_, unsigned char *data_);
    RhiTexture(Format format_, const Vec2& pixelSize_, int sampleCount_, Flag flags_, const std::array<unsigned char*, 6>& cube_datas_);
    Format m_format;
    Vec2 m_pixelSize;
    int m_sampleCount;
    Flag m_flags;
    unsigned char *m_data{nullptr};
    std::array<unsigned char*, 6> m_cube_datas{};
    GL_HANDLE m_id{0};
};

class RhiAttachment
{
public:
    enum Type {
        Color,
        Depth,
        DepthStencil,
    };

    RhiAttachment() = default;
    RhiAttachment(RhiTexture *texture, int layer = 0, int level = 0, bool owns_texture = true);
    RhiTexture *texture() const { return m_texture; }
    int layer() const { return m_layer; }
    int level() const { return m_level; }
    void release(); // destroy texture GPU data and delete object

protected:
    RhiTexture *m_texture{nullptr};
    int m_layer = 0; // cube map face
    int m_level = 0; // mipmap level
    bool m_owns_texture = true;
};

// 完整 attachment 描述：绑定位置信息、纹理格式、采样数和生命周期描述。
struct RhiAttachmentDesc {
    RhiAttachment::Type attachment_type{ RhiAttachment::Type::Color };
    int color_attachment_index{ 0 };

    RhiTexture::Format format{ RhiTexture::Format::UnknownFormat };
    int sample_count{ 1 };
    bool transient{ true };

    bool isSameWith(const RhiAttachmentDesc& desc) const {
        return attachment_type == desc.attachment_type &&
            color_attachment_index == desc.color_attachment_index &&
            format == desc.format &&
            sample_count == desc.sample_count &&
            transient == desc.transient;
    }
};

// framebuffer 创建所需的 RhiAttachmentDesc 集合。
struct RhiFrameBufferDesc {
    Vec2 size;
    std::array<bool, 8> has_color{};
    std::array<RhiAttachmentDesc, 8> colors{};
    bool has_depth{ false };
    RhiAttachmentDesc depth;
    bool has_depth_stencil{ false };
    RhiAttachmentDesc depth_stencil;

    bool isEmpty() const {
        if (has_depth || has_depth_stencil)
            return false;
        return std::none_of(has_color.begin(), has_color.end(), [](bool has_color) { return has_color; });
    }

    bool isSameWith(const RhiFrameBufferDesc& desc) const {
        if (size != desc.size ||
            has_color != desc.has_color ||
            has_depth != desc.has_depth ||
            has_depth_stencil != desc.has_depth_stencil)
            return false;

        for (size_t i = 0; i < colors.size(); ++i)
        {
            if (has_color[i] && !colors[i].isSameWith(desc.colors[i]))
                return false;
        }
        if (has_depth && !depth.isSameWith(desc.depth))
            return false;
        if (has_depth_stencil && !depth_stencil.isSameWith(desc.depth_stencil))
            return false;
        return true;
    }
};

class RhiFrameBuffer // 别名RhiTextureRenderTarget
{
public:
    virtual ~RhiFrameBuffer() = default;

    Vec2 pixelSize() const { return m_pixelSize; }
    void setPixelSize(const Vec2 &sz) { m_pixelSize = sz; }

    void setSampleCount(int sampleCount_) { m_sampleCount = sampleCount_; }
    int sampleCount() const { return m_sampleCount; }

    GL_HANDLE id() const { return m_id; }

    const RhiAttachment *colorAttachmentAt(int index) const { return &m_colorAttachments.at(index); }
    void setColorAttachments(std::initializer_list<RhiAttachment> list)
    {
        std::array<RhiAttachment, 8> attachments;
        int i = 0;
        for (auto it = list.begin(); it != list.end(); ++it)
        {
            if (i < attachments.size())
                attachments[i++] = *it;
        }
        m_colorAttachments.swap(attachments);
    }
    void setColorAttachments(const std::array<RhiAttachment, 8> &list) { m_colorAttachments = list; }

    const RhiAttachment *depthAttachment() const { return &m_depthAttachment; }
    void setDepthAttachment(RhiAttachment depthAttachment_) { m_depthAttachment = depthAttachment_; }

    const RhiAttachment *depthStencilAttachment() const { return &m_depthStencilAttachment; }
    void setDepthStencilAttachment(RhiAttachment depthStencilAttachment_) { m_depthStencilAttachment = depthStencilAttachment_; }

    virtual bool create() = 0; // truely generate and bind a frameBuffer
    virtual void bind() = 0;
    virtual void unBind() = 0;
    virtual void clear(Color4 clear_color = Color4(0.f, 0.f, 0.f, 1.0f)) = 0;
    virtual void blitTo(RhiFrameBuffer *dest, RhiTexture::Format format = RhiTexture::Format::RGBA16F) = 0;
    virtual void destroyGPU(); // delete FBO and attachment textures (OpenGL impl)

protected:
    RhiFrameBuffer() = default;
    RhiFrameBuffer(const RhiAttachment &colorAttachment, const Vec2 &pixelSize_, int sampleCount_ = 1);
    std::array<RhiAttachment, 8> m_colorAttachments;
    RhiAttachment m_depthAttachment;
    RhiAttachment m_depthStencilAttachment;
    Vec2 m_pixelSize;
    int m_sampleCount{1};
    GL_HANDLE m_id{0};
};

struct RhiVertexAttribute
{
    enum Format
    {
        Float4,
        Float3,
        Float2,
        Float,
        UInt4,
        UInt3,
        UInt2,
        UInt,
        SInt4,
        SInt3,
        SInt2,
        SInt
    };

    int location = -1;
    Format format = Float4;
    int stride = 0;
    int offset = 0;
};

class RhiVertexLayout
{
public:
    void setAttributes(std::initializer_list<RhiVertexAttribute> list)
    {
        m_attributes.assign(list.begin(), list.end());
    }
    template <typename InputIterator>
    void setAttributes(InputIterator first, InputIterator last)
    {
        m_attributes.assign(first, last);
    }
    const RhiVertexAttribute *cbeginAttributes() const { return m_attributes.empty() ? nullptr : m_attributes.data(); }
    const RhiVertexAttribute *cendAttributes() const { return m_attributes.empty() ? nullptr : m_attributes.data() + m_attributes.size(); }

    GL_HANDLE id() const { return m_id; }

    virtual bool create() = 0;

    virtual bool createInstancing(RhiBuffer *inst_buffer, int instancin_location) = 0;

protected:
    RhiVertexLayout() = default;
    RhiVertexLayout(RhiBuffer *vbuffer, RhiBuffer *ibuffer);
    std::vector<RhiVertexAttribute> m_attributes;
    RhiBuffer *m_vbuffer;
    RhiBuffer *m_ibuffer;
    GL_HANDLE m_id;
};

// 一段 shader stage 的输入描述。
//
// Qt RHI 里 shader 是 pipeline 的一部分，而不是 pass 里随手 bind 的独立对象。
// 这里先采用 OpenGL-only 的精简形态：直接保存 GLSL 源码和调试名，由
// RhiGraphicsPipeline::create() 的后端实现负责编译并链接成 native program。
class RhiShaderStage
{
public:
    enum Type
    {
        Vertex,
        Fragment,
        Geometry
    };

    RhiShaderStage() = default;
    RhiShaderStage(Type type_, std::string source_, std::string debug_name_ = {});

    Type type() const { return m_type; }
    const std::string& source() const { return m_source; }
    const std::string& debugName() const { return m_debug_name; }

private:
    Type m_type{ Vertex };
    std::string m_source;
    std::string m_debug_name;
};

// ShaderResourceBindings 是 draw 前要暴露给 shader 的动态资源集合。
// 当前 OpenGL-only 版本先保留 name-based uniform/texture 绑定，避免过早引入
// descriptor set / bind group layout 的复杂度；CommandBuffer 会在绑定了
// GraphicsPipeline 之后，把这些条目应用到当前 native shader program。
class RhiShaderResourceBindings
{
public:
    enum Type
    {
        Bool,
        Int,
        Float,
        Float3,
        Float4,
        Matrix,
        Texture2D,
        TextureCube
    };

    struct Binding
    {
        std::string name;
        Type type{ Int };
        int int_value{ 0 };
        float values[4]{ 0.f, 0.f, 0.f, 0.f };
        int texture_unit{ 0 };
        RhiTexture* texture{ nullptr };
        std::vector<Mat4> matrices;
    };

    void clear();
    void setBool(const std::string& name, bool value);
    void setInt(const std::string& name, int value);
    void setFloat(const std::string& name, float value);
    void setFloat3(const std::string& name, const Vec3& value);
    void setFloat4(const std::string& name, float value1, float value2, float value3, float value4);
    void setFloat4(const std::string& name, const Vec4& value);
    void setMatrix(const std::string& name, int count, const Mat4& mat_value);
    void setTexture(const std::string& name, int texture_unit, RhiTexture* texture);
    void setTexture(const std::string& name, int texture_unit, RhiTexture& texture) { setTexture(name, texture_unit, &texture); }
    void setCubeTexture(const std::string& name, int texture_unit, RhiTexture* texture);
    void setCubeTexture(const std::string& name, int texture_unit, RhiTexture& texture) { setCubeTexture(name, texture_unit, &texture); }

    const std::vector<Binding>& bindings() const { return m_bindings; }

private:
    Binding& upsert(const std::string& name, Type type);

    std::vector<Binding> m_bindings;
};

using ShaderResourceBindings = RhiShaderResourceBindings;

// GraphicsPipeline 表示“一类 draw call 的固定渲染配方”。
//
// 它不是某个 Mesh，也不是某一次 draw。它收束的是过去分散在 RenderPass 中的
// shader program、primitive topology、cull/front face、blend、depth/stencil、
// vertex input layout 等固定状态。引入它之后，pass 中不应该再零散地调用
// setBlend()/setDepthMask()/setFrontFaceCW() 来临时拼状态，而是切换到描述完整的
// pipeline。
//
// 当前项目长期只支持 OpenGL，所以这个抽象比 Qt QRhiGraphicsPipeline 精简：
// - 暂不包含跨后端 render-pass compatibility 对象；
// - 暂不包含 pipeline cache；
// - 暂不包含 compute/tessellation 的复杂路径；
// - viewport/scissor/blend constants/stencil ref 仍作为 CommandBuffer 的动态状态。
class RhiGraphicsPipeline
{
public:
    virtual ~RhiGraphicsPipeline() = default;

    enum Flag
    {
        UsesBlendConstants = 1 << 0,
        UsesStencilRef = 1 << 1,
        UsesScissor = 1 << 2,
        CompileShadersWithDebugInfo = 1 << 3
    };

    enum Topology
    {
        Triangles,
        TriangleStrip,
        TriangleFan,
        Lines,
        LineStrip,
        Points
    };

    enum CullMode
    {
        None,
        Front,
        Back
    };

    enum FrontFace
    {
        CCW,
        CW
    };

    enum ColorMask
    {
        R = 1 << 0,
        G = 1 << 1,
        B = 1 << 2,
        A = 1 << 3
    };

    enum BlendFactor
    {
        Zero,
        One,
        SrcColor,
        OneMinusSrcColor,
        DstColor,
        OneMinusDstColor,
        SrcAlpha,
        OneMinusSrcAlpha,
        DstAlpha,
        OneMinusDstAlpha,
        ConstantColor,
        OneMinusConstantColor,
        ConstantAlpha,
        OneMinusConstantAlpha,
        SrcAlphaSaturate
    };

    enum BlendOp
    {
        Add,
        Subtract,
        ReverseSubtract,
        Min,
        Max
    };

    struct TargetBlend
    {
        int colorWrite{ R | G | B | A };
        bool enable{ false };
        BlendFactor srcColor{ One };
        BlendFactor dstColor{ OneMinusSrcAlpha };
        BlendOp opColor{ Add };
        BlendFactor srcAlpha{ One };
        BlendFactor dstAlpha{ OneMinusSrcAlpha };
        BlendOp opAlpha{ Add };
    };

    enum CompareOp
    {
        Never,
        Less,
        Equal,
        LessOrEqual,
        Greater,
        NotEqual,
        GreaterOrEqual,
        Always
    };

    enum StencilOp
    {
        StencilZero,
        Keep,
        Replace,
        IncrementAndClamp,
        DecrementAndClamp,
        Invert,
        IncrementAndWrap,
        DecrementAndWrap
    };

    struct StencilOpState
    {
        StencilOp failOp{ Keep };
        StencilOp depthFailOp{ Keep };
        StencilOp passOp{ Keep };
        CompareOp compareOp{ Always };
    };

    int flags() const { return m_flags; }
    void setFlags(int f) { m_flags = f; }

    Topology topology() const { return m_topology; }
    void setTopology(Topology t) { m_topology = t; }

    CullMode cullMode() const { return m_cullMode; }
    void setCullMode(CullMode mode) { m_cullMode = mode; }

    FrontFace frontFace() const { return m_frontFace; }
    void setFrontFace(FrontFace f) { m_frontFace = f; }

    void setTargetBlends(std::initializer_list<TargetBlend> list)
    {
        m_targetBlends.assign(list.begin(), list.end());
    }
    template <typename InputIterator>
    void setTargetBlends(InputIterator first, InputIterator last)
    {
        m_targetBlends.assign(first, last);
    }
    const TargetBlend* cbeginTargetBlends() const { return m_targetBlends.empty() ? nullptr : m_targetBlends.data(); }
    const TargetBlend* cendTargetBlends() const { return m_targetBlends.empty() ? nullptr : m_targetBlends.data() + m_targetBlends.size(); }
    const std::vector<TargetBlend>& targetBlends() const { return m_targetBlends; }

    bool hasDepthTest() const { return m_depthTest; }
    void setDepthTest(bool enable) { m_depthTest = enable; }

    bool hasDepthWrite() const { return m_depthWrite; }
    void setDepthWrite(bool enable) { m_depthWrite = enable; }

    CompareOp depthOp() const { return m_depthOp; }
    void setDepthOp(CompareOp op) { m_depthOp = op; }

    bool hasStencilTest() const { return m_stencilTest; }
    void setStencilTest(bool enable) { m_stencilTest = enable; }

    StencilOpState stencilFront() const { return m_stencilFront; }
    void setStencilFront(const StencilOpState& state) { m_stencilFront = state; }

    StencilOpState stencilBack() const { return m_stencilBack; }
    void setStencilBack(const StencilOpState& state) { m_stencilBack = state; }

    int stencilReadMask() const { return m_stencilReadMask; }
    void setStencilReadMask(int mask) { m_stencilReadMask = mask; }

    int stencilWriteMask() const { return m_stencilWriteMask; }
    void setStencilWriteMask(int mask) { m_stencilWriteMask = mask; }

    int sampleCount() const { return m_sampleCount; }
    void setSampleCount(int s) { m_sampleCount = s; }

    float lineWidth() const { return m_lineWidth; }
    void setLineWidth(float width) { m_lineWidth = width; }

    int depthBias() const { return m_depthBias; }
    void setDepthBias(int bias) { m_depthBias = bias; }

    float slopeScaledDepthBias() const { return m_slopeScaledDepthBias; }
    void setSlopeScaledDepthBias(float bias) { m_slopeScaledDepthBias = bias; }

    void setShaderStages(std::initializer_list<RhiShaderStage> list)
    {
        m_shaderStages.assign(list.begin(), list.end());
    }
    template <typename InputIterator>
    void setShaderStages(InputIterator first, InputIterator last)
    {
        m_shaderStages.assign(first, last);
    }
    const RhiShaderStage* cbeginShaderStages() const { return m_shaderStages.empty() ? nullptr : m_shaderStages.data(); }
    const RhiShaderStage* cendShaderStages() const { return m_shaderStages.empty() ? nullptr : m_shaderStages.data() + m_shaderStages.size(); }
    const std::vector<RhiShaderStage>& shaderStages() const { return m_shaderStages; }

    RhiVertexLayout* vertexInputLayout() const { return m_vertexInputLayout; }
    void setVertexInputLayout(RhiVertexLayout* layout) { m_vertexInputLayout = layout; }

    // OpenGL-only 阶段仍有少量旧代码需要直接设置 uniform。
    // 这里暴露的是后端 native shader program 的只读句柄；上层 RenderPass 不应拿它来做 draw，
    // draw 入口统一走 CommandBuffer。
    virtual GL_HANDLE id() const { return 0; }

    // 后端在 create() 中把描述转换成 native object。
    // OpenGL 后端会在这里编译 shader 并链接 program。
    virtual bool create() = 0;

protected:
    RhiGraphicsPipeline() = default;

    int m_flags{ 0 };
    Topology m_topology{ Triangles };
    CullMode m_cullMode{ Back };
    FrontFace m_frontFace{ CCW };
    std::vector<TargetBlend> m_targetBlends;
    bool m_depthTest{ true };
    bool m_depthWrite{ true };
    CompareOp m_depthOp{ LessOrEqual };
    bool m_stencilTest{ false };
    StencilOpState m_stencilFront;
    StencilOpState m_stencilBack;
    int m_stencilReadMask{ 0xFF };
    int m_stencilWriteMask{ 0xFF };
    int m_sampleCount{ 1 };
    float m_lineWidth{ 1.0f };
    int m_depthBias{ 0 };
    float m_slopeScaledDepthBias{ 0.0f };
    std::vector<RhiShaderStage> m_shaderStages;
    RhiVertexLayout* m_vertexInputLayout{ nullptr };
};

// CommandBuffer 是 RenderPass 和后端 API 之间的命令接口。
//
// Qt 的 RHI 会把命令记录下来，在 endFrame/finish 时统一提交。项目目前只支持
// OpenGL，因此第一版采用“立即执行式 CommandBuffer”：接口长得像命令缓冲，
// 但 OpenGL 后端在调用时就立刻执行 glBindFramebuffer/glUseProgram/glDraw*。
//
// 这样做的意义不是为了模拟 Vulkan，而是给上层建立清晰边界：
// 1. beginPass/endPass 明确 render target 和清屏时机；
// 2. draw 前必须显式绑定 GraphicsPipeline；
// 3. vertex/index input、viewport、scissor、dynamic state 有统一入口；
// 4. 后续如果想切换为真正录制命令，上层 RenderPass 不需要再改 API。
class RhiCommandBuffer
{
public:
    virtual ~RhiCommandBuffer() = default;

    enum IndexFormat
    {
        IndexUInt16,
        IndexUInt32
    };

    virtual void beginPass(RhiFrameBuffer* render_target,
                           const Color4& color_clear_value = Color4(0.f, 0.f, 0.f, 1.f),
                           float depth_clear_value = 1.0f,
                           int stencil_clear_value = 0,
                           bool clear_color = true,
                           bool clear_depth_stencil = true) = 0;
    virtual void endPass() = 0;

    virtual void setGraphicsPipeline(RhiGraphicsPipeline* pipeline) = 0;
    virtual void setShaderResources(RhiShaderResourceBindings* bindings = nullptr) = 0;
    virtual void setVertexInput(RhiVertexLayout* layout,
                                RhiBuffer* index_buffer = nullptr,
                                int index_offset = 0,
                                IndexFormat index_format = IndexUInt32) = 0;
    virtual void setViewport(int x, int y, int width, int height) = 0;
    virtual void setScissor(int x, int y, int width, int height) = 0;
    virtual void setBlendConstants(const Color4& c) = 0;
    virtual void setStencilRef(int ref_value) = 0;
    virtual void blit(RhiFrameBuffer* source,
                      RhiFrameBuffer* dest,
                      RhiTexture::Format format = RhiTexture::Format::RGBA16F) = 0;

    virtual void draw(int vertex_count,
                      int instance_count = 1,
                      int first_vertex = 0,
                      int first_instance = 0) = 0;
    virtual void drawIndexed(int index_count,
                             int instance_count = 1,
                             int first_index = 0,
                             int vertex_offset = 0,
                             int first_instance = 0) = 0;

protected:
    RhiCommandBuffer() = default;
};

class Rhi
{
public:
    static Rhi*& get();
    static Rhi* create();

    // render相关
    void drawIndexed(GL_HANDLE vao_id, size_t indices_count, size_t index_offset = 0, int inst_amount = -1);
    void drawTriangles(GL_HANDLE vao_id, size_t array_count);

    // context 全局状态
    void setViewport(int x, int y, int width, int height);
    void setBlend(bool enable);
    void setDepthMask(bool enable);
    void setFrontFaceCW(bool cw);

    // framebuffer 管理
    RhiFrameBuffer* newFrameBuffer(const RhiAttachment& colorAttachment, const Vec2& pixelSize_, int sampleCount_ = 1);
    void readPixelRGBA(GL_HANDLE framebuffer, int x, int y, unsigned char out_rgba[4]);

    //// binding resource
    // virtual void bindTexture(/*TextureData*/) = 0;
    // virtual void bindVertexArray() = 0;
    // virtual void bindBuffer(/*BufferData*/) = 0;

    // texture
    RhiTexture* newTexture(RhiTexture::Format format,
        const Vec2& pixelSize,
        int sampleCount = 1,
        RhiTexture::Flag flags = {},
        unsigned char* data = nullptr);
    RhiTexture* newCubeTexture(RhiTexture::Format format,
        const Vec2& pixelSize,
        int sampleCount,
        RhiTexture::Flag flags,
        const std::array<unsigned char*, 6>& cube_datas);

    RhiBuffer *newBuffer(RhiBuffer::Type type,
                         RhiBuffer::UsageFlag usage,
                         void *data,
                         int size);

    RhiVertexLayout *newVertexLayout(RhiBuffer *vbuffer, RhiBuffer *ibuffer);

    RhiGraphicsPipeline* newGraphicsPipeline();
    RhiCommandBuffer* newCommandBuffer();

private:
    RhiImpl* m_impl{ nullptr };
};
