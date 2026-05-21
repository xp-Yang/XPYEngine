#ifndef rhi_opengl_hpp
#define rhi_opengl_hpp

#include <glad/glad.h>
// must place behind <glad/glad.h>,
// otherwise: "#error: OpenGL header already included, remove this include, glad already provides it"
#include <GLFW/glfw3.h>
#include "Render/RHI/rhi_impl.hpp"

class RhiOpenGL : public RhiImpl
{
public:
    RhiOpenGL();
    ~RhiOpenGL() override {};

    // render相关
    void drawIndexed(GL_HANDLE vao_id, size_t indices_count, size_t index_offset = 0, int inst_amount = -1) override;
    void drawTriangles(GL_HANDLE vao_id, size_t array_count) override;

    // context 全局状态
    void setViewport(int x, int y, int width, int height) override;
    void setBlend(bool enable) override;
    void setDepthMask(bool enable) override;
    void setFrontFaceCW(bool cw) override;

    // framebuffer 管理
    RhiFrameBuffer *newFrameBuffer(const RhiAttachment &colorAttachment, const Vec2 &pixelSize_, int sampleCount_ = 1) override;
    void readPixelRGBA(GL_HANDLE framebuffer, int x, int y, unsigned char out_rgba[4]) override;

    //// binding resource
    // void bindTexture(/*TextureData*/) override;
    // void bindVertexArray() override;
    // void bindBuffer(/*BufferData*/) override;

    RhiTexture* newTexture(RhiTexture::Format format,
        const Vec2& pixelSize,
        int sampleCount = 1,
        RhiTexture::Flag flags = {},
        unsigned char* data = nullptr) override;
    RhiTexture* newCubeTexture(RhiTexture::Format format,
        const Vec2& pixelSize,
        int sampleCount,
        RhiTexture::Flag flags,
        const std::array<unsigned char*, 6>& cube_datas) override;

    RhiBuffer *newBuffer(RhiBuffer::Type type,
                         RhiBuffer::UsageFlag usage,
                         void *data,
                         int size) override;

    RhiVertexLayout *newVertexLayout(RhiBuffer *vbuffer, RhiBuffer *ibuffer) override;

    RhiGraphicsPipeline *newGraphicsPipeline() override;
    RhiCommandBuffer *newCommandBuffer() override;
};

#endif
