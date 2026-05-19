#pragma once

#include <Base/Common.hpp>

class RhiResource;
class RhiBuffer;
class RhiTexture;
class RhiAttachment;
class RhiFrameBuffer;
class RhiCommandBuffer;
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

    unsigned int id() const { return m_id; }

    virtual bool create() = 0;
    virtual void update(void* data, int size, int offset = 0) = 0;

protected:
    RhiBuffer(Type type_, UsageFlag usage_, void *data, int size_);
    Type m_type;
    UsageFlag m_usage;
    void *m_data{nullptr};
    int m_size;
    unsigned int m_id{0};
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

    unsigned int id() const { return m_id; }

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
    unsigned int m_id{0};
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

    unsigned int id() const { return m_id; }

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
    unsigned int m_id{0};
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

    unsigned int id() const { return m_id; }

    virtual bool create() = 0;

    virtual bool createInstancing(RhiBuffer *inst_buffer, int instancin_location) = 0;

protected:
    RhiVertexLayout() = default;
    RhiVertexLayout(RhiBuffer *vbuffer, RhiBuffer *ibuffer);
    std::vector<RhiVertexAttribute> m_attributes;
    RhiBuffer *m_vbuffer;
    RhiBuffer *m_ibuffer;
    unsigned int m_id;
};

class Rhi
{
public:
    static Rhi*& get();
    static Rhi* create();

    // render相关
    void drawIndexed(unsigned int vao_id, size_t indices_count, size_t index_offset = 0, int inst_amount = -1);
    void drawTriangles(unsigned int vao_id, size_t array_count);

    // context 全局状态
    void setViewport(int x, int y, int width, int height);
    void setBlend(bool enable);
    void setDepthMask(bool enable);
    void setFrontFaceCW(bool cw);

    // framebuffer 管理
    RhiFrameBuffer* newFrameBuffer(const RhiAttachment& colorAttachment, const Vec2& pixelSize_, int sampleCount_ = 1);
    void readPixelRGBA(unsigned int framebuffer, int x, int y, unsigned char out_rgba[4]);

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

private:
    RhiImpl* m_impl{ nullptr };
};

//class RhiCommandBuffer
//{
//public:
//    enum IndexFormat
//    {
//        IndexUInt16,
//        IndexUInt32
//    };
//
//    enum BeginPassFlag
//    {
//        ExternalContent = 0x01,
//        DoNotTrackResourcesForCompute = 0x02
//    };
//
//    // void resourceUpdate(QRhiResourceUpdateBatch* resourceUpdates);
//
//    // void beginPass(RhiRenderTarget* rt,
//    //     const Color4& colorClearValue,
//    //     const QRhiDepthStencilClearValue& depthStencilClearValue,
//    //     QRhiResourceUpdateBatch* resourceUpdates = nullptr,
//    //     BeginPassFlags flags = {});
//    // void endPass(QRhiResourceUpdateBatch* resourceUpdates = nullptr);
//
//    // using VertexInput = std::pair<RhiBuffer*, int>; // buffer, offset
//    // void setVertexInput(int startBinding, int bindingCount, const VertexInput* bindings,
//    //     RhiBuffer* indexBuf = nullptr, int indexOffset = 0,
//    //     IndexFormat indexFormat = IndexUInt16);
//
//    // void setViewport(const RhiViewport& viewport);
//    // void setScissor(const RhiScissor& scissor);
//    void setBlendConstants(const Color4& c);
//    void setStencilRef(int refValue);
//
//    void draw(int vertexCount,
//        int instanceCount = 1,
//        int firstVertex = 0,
//        int firstInstance = 0);
//
//    void drawIndexed(int indexCount,
//        int instanceCount = 1,
//        int firstIndex = 0,
//        int vertexOffset = 0,
//        int firstInstance = 0);
//
//protected:
//    RhiCommandBuffer();
//};
//
//class RhiSwapChain
//{
//public:
//    enum Flag
//    {
//        sRGB = 1 << 2,
//        UsedAsTransferSource = 1 << 3,
//        NoVSync = 1 << 4,
//        MinimalBufferCount = 1 << 5
//    };
//
//    enum Format
//    {
//        SDR,
//        HDRExtendedSrgbLinear,
//        HDR10
//    };
//
//    enum StereoTargetBuffer
//    {
//        LeftBuffer,
//        RightBuffer
//    };
//
//    Window* window() const { return m_window; }
//    void setWindow(Window* window) { m_window = window; }
//
//    Flag flags() const { return m_flags; }
//    void setFlags(Flag f) { m_flags = f; }
//
//    Format format() const { return m_format; }
//    void setFormat(Format f) { m_format = f; }
//
//    const RhiAttachment* depthStencil() const { return &m_depthStencilAttachment; }
//    void setDepthStencil(RhiAttachment depthStencilAttachment_) { m_depthStencilAttachment = depthStencilAttachment_; }
//
//    int sampleCount() const { return m_sampleCount; }
//    void setSampleCount(int samples) { m_sampleCount = samples; }
//
//    Vec2 currentPixelSize() const { return m_currentPixelSize; }
//
//    virtual RhiCommandBuffer* currentFrameCommandBuffer() = 0;
//    // virtual RhiRenderTarget* currentFrameRenderTarget() = 0;
//    // virtual RhiRenderTarget* currentFrameRenderTarget(StereoTargetBuffer targetBuffer);
//    virtual Vec2 surfacePixelSize() = 0;
//    virtual bool isFormatSupported(Format f) = 0;
//    virtual bool createOrResize() = 0;
//
//protected:
//    RhiSwapChain();
//    Window* m_window = nullptr;
//    Flag m_flags;
//    Format m_format = SDR;
//    RhiAttachment m_depthStencilAttachment;
//    int m_sampleCount = 1;
//    Vec2 m_currentPixelSize;
//};
//
//class RhiGraphicsPipeline
//{
//public:
//    enum Flag
//    {
//        UsesBlendConstants = 1 << 0,
//        UsesStencilRef = 1 << 1,
//        UsesScissor = 1 << 2,
//        CompileShadersWithDebugInfo = 1 << 3
//    };
//
//    enum Topology
//    {
//        Triangles,
//        TriangleStrip,
//        TriangleFan,
//        Lines,
//        LineStrip,
//        Points,
//        Patches
//    };
//
//    enum CullMode
//    {
//        None,
//        Front,
//        Back
//    };
//
//    enum FrontFace
//    {
//        CCW,
//        CW
//    };
//
//    enum ColorMask
//    {
//        R = 1 << 0,
//        G = 1 << 1,
//        B = 1 << 2,
//        A = 1 << 3
//    };
//
//    enum BlendFactor
//    {
//        Zero,
//        One,
//        SrcColor,
//        OneMinusSrcColor,
//        DstColor,
//        OneMinusDstColor,
//        SrcAlpha,
//        OneMinusSrcAlpha,
//        DstAlpha,
//        OneMinusDstAlpha,
//        ConstantColor,
//        OneMinusConstantColor,
//        ConstantAlpha,
//        OneMinusConstantAlpha,
//        SrcAlphaSaturate,
//        Src1Color,
//        OneMinusSrc1Color,
//        Src1Alpha,
//        OneMinusSrc1Alpha
//    };
//
//    enum BlendOp
//    {
//        Add,
//        Subtract,
//        ReverseSubtract,
//        Min,
//        Max
//    };
//
//    struct TargetBlend
//    {
//        ColorMask colorWrite = ColorMask(0xF); // R | G | B | A
//        bool enable = false;
//        BlendFactor srcColor = One;
//        BlendFactor dstColor = OneMinusSrcAlpha;
//        BlendOp opColor = Add;
//        BlendFactor srcAlpha = One;
//        BlendFactor dstAlpha = OneMinusSrcAlpha;
//        BlendOp opAlpha = Add;
//    };
//
//    enum CompareOp
//    {
//        Never,
//        Less,
//        Equal,
//        LessOrEqual,
//        Greater,
//        NotEqual,
//        GreaterOrEqual,
//        Always
//    };
//
//    enum StencilOp
//    {
//        StencilZero,
//        Keep,
//        Replace,
//        IncrementAndClamp,
//        DecrementAndClamp,
//        Invert,
//        IncrementAndWrap,
//        DecrementAndWrap
//    };
//
//    struct StencilOpState
//    {
//        StencilOp failOp = Keep;
//        StencilOp depthFailOp = Keep;
//        StencilOp passOp = Keep;
//        CompareOp compareOp = Always;
//    };
//
//    Flag flags() const { return m_flags; }
//    void setFlags(Flag f) { m_flags = f; }
//
//    Topology topology() const { return m_topology; }
//    void setTopology(Topology t) { m_topology = t; }
//
//    CullMode cullMode() const { return m_cullMode; }
//    void setCullMode(CullMode mode) { m_cullMode = mode; }
//
//    FrontFace frontFace() const { return m_frontFace; }
//    void setFrontFace(FrontFace f) { m_frontFace = f; }
//
//    template <typename InputIterator>
//    void setTargetBlends(InputIterator first, InputIterator last)
//    {
//        m_targetBlends.clear();
//        std::copy(first, last, std::back_inserter(m_targetBlends));
//    }
//    const TargetBlend *cbeginTargetBlends() const { return m_targetBlends.data(); }
//    const TargetBlend *cendTargetBlends() const { return m_targetBlends.data() + m_targetBlends.size(); }
//
//    bool hasDepthTest() const { return m_depthTest; }
//    void setDepthTest(bool enable) { m_depthTest = enable; }
//
//    bool hasDepthWrite() const { return m_depthWrite; }
//    void setDepthWrite(bool enable) { m_depthWrite = enable; }
//
//    CompareOp depthOp() const { return m_depthOp; }
//    void setDepthOp(CompareOp op) { m_depthOp = op; }
//
//    bool hasStencilTest() const { return m_stencilTest; }
//    void setStencilTest(bool enable) { m_stencilTest = enable; }
//
//    StencilOpState stencilFront() const { return m_stencilFront; }
//    void setStencilFront(const StencilOpState &state) { m_stencilFront = state; }
//
//    StencilOpState stencilBack() const { return m_stencilBack; }
//    void setStencilBack(const StencilOpState &state) { m_stencilBack = state; }
//
//    int stencilReadMask() const { return m_stencilReadMask; }
//    void setStencilReadMask(int mask) { m_stencilReadMask = mask; }
//
//    int stencilWriteMask() const { return m_stencilWriteMask; }
//    void setStencilWriteMask(int mask) { m_stencilWriteMask = mask; }
//
//    int sampleCount() const { return m_sampleCount; }
//    void setSampleCount(int s) { m_sampleCount = s; }
//
//    float lineWidth() const { return m_lineWidth; }
//    void setLineWidth(float width) { m_lineWidth = width; }
//
//    int depthBias() const { return m_depthBias; }
//    void setDepthBias(int bias) { m_depthBias = bias; }
//
//    float slopeScaledDepthBias() const { return m_slopeScaledDepthBias; }
//    void setSlopeScaledDepthBias(float bias) { m_slopeScaledDepthBias = bias; }
//
//    // void setShaderStages(std::initializer_list<QRhiShaderStage> list) { m_shaderStages = list; }
//    // template<typename InputIterator>
//    // void setShaderStages(InputIterator first, InputIterator last)
//    //{
//    //     m_shaderStages.clear();
//    //     std::copy(first, last, std::back_inserter(m_shaderStages));
//    // }
//    // const RhiShaderStage* cbeginShaderStages() const { return m_shaderStages.cbegin(); }
//    // const RhiShaderStage* cendShaderStages() const { return m_shaderStages.cend(); }
//
//    RhiVertexLayout *vertexInputLayout() const { return m_vertexInputLayout; }
//    void setVertexInputLayout(RhiVertexLayout *layout) { m_vertexInputLayout = layout; }
//
//    // RhiShaderResourceBindings* shaderResourceBindings() const { return m_shaderResourceBindings; }
//    // void setShaderResourceBindings(RhiShaderResourceBindings* srb) { m_shaderResourceBindings = srb; }
//
//    // RhiRenderPassDescriptor* renderPassDescriptor() const { return m_renderPassDesc; }
//    // void setRenderPassDescriptor(RhiRenderPassDescriptor* desc) { m_renderPassDesc = desc; }
//
//    int patchControlPointCount() const { return m_patchControlPointCount; }
//    void setPatchControlPointCount(int count) { m_patchControlPointCount = count; }
//
//    virtual bool create() = 0;
//
//protected:
//    RhiGraphicsPipeline();
//    Flag m_flags;
//    Topology m_topology = Triangles;
//    CullMode m_cullMode = None;
//    FrontFace m_frontFace = CCW;
//    std::vector<TargetBlend> m_targetBlends;
//    bool m_depthTest = false;
//    bool m_depthWrite = false;
//    CompareOp m_depthOp = Less;
//    bool m_stencilTest = false;
//    StencilOpState m_stencilFront;
//    StencilOpState m_stencilBack;
//    int m_stencilReadMask = 0xFF;
//    int m_stencilWriteMask = 0xFF;
//    int m_sampleCount = 1;
//    float m_lineWidth = 1.0f;
//    int m_depthBias = 0;
//    float m_slopeScaledDepthBias = 0.0f;
//    int m_patchControlPointCount = 3;
//    // std::array<RhiShaderStage, 4> m_shaderStages;
//    RhiVertexLayout *m_vertexInputLayout;
//    // RhiShaderResourceBindings* m_shaderResourceBindings = nullptr;
//    // RhiRenderPassDescriptor* m_renderPassDesc = nullptr;
//};
