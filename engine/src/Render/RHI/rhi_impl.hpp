#ifndef rhi_impl_hpp
#define rhi_impl_hpp

#include "rhi.hpp"

class RhiImpl
{
public:
    virtual ~RhiImpl() {};

    // render相关
    virtual void drawIndexed(unsigned int vao_id, size_t indices_count, size_t index_offset = 0, int inst_amount = -1) = 0;
    virtual void drawTriangles(unsigned int vao_id, size_t array_count) = 0;

    // context 全局状态
    virtual void setViewport(int x, int y, int width, int height) = 0;
    virtual void setBlend(bool enable) = 0;
    virtual void setDepthMask(bool enable) = 0;
    virtual void setFrontFaceCW(bool cw) = 0;

    // framebuffer 管理
    virtual RhiFrameBuffer* newFrameBuffer(const RhiAttachment& colorAttachment, const Vec2& pixelSize_, int sampleCount_ = 1) = 0;
    virtual void readPixelRGBA(unsigned int framebuffer, int x, int y, unsigned char out_rgba[4]) = 0;

    //// binding resource
    // virtual void bindTexture(/*TextureData*/) = 0;
    // virtual void bindVertexArray() = 0;
    // virtual void bindBuffer(/*BufferData*/) = 0;

    virtual RhiTexture* newTexture(RhiTexture::Format format,
        const Vec2& pixelSize,
        int sampleCount = 1,
        RhiTexture::Flag flags = {},
        unsigned char* data = nullptr) = 0;
    virtual RhiTexture* newCubeTexture(RhiTexture::Format format,
        const Vec2& pixelSize,
        int sampleCount,
        RhiTexture::Flag flags,
        const std::array<unsigned char*, 6>& cube_datas) = 0;

    virtual RhiBuffer *newBuffer(RhiBuffer::Type type,
                                 RhiBuffer::UsageFlag usage,
                                 void *data,
                                 int size) = 0;

    virtual RhiVertexLayout *newVertexLayout(RhiBuffer *vbuffer, RhiBuffer *ibuffer) = 0;
};

#endif
