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
    void drawIndexed(unsigned int vao_id, size_t indices_count, int inst_amount = -1) override;
    void drawTriangles(unsigned int vao_id, size_t array_count) override;
    // context 全局状态
    void setViewport(int x, int y, int width, int height) override;
    void setDepthMask(bool enable) override;
    void setFrontFaceCW(bool cw) override;
    void readPixelRGBA(unsigned int framebuffer, int x, int y, unsigned char out_rgba[4]) override;
    unsigned int newFramebufferHandle() override;
    void bindFramebuffer(unsigned int framebuffer) override;
    void bindDefaultFramebuffer() override;
    void setFramebufferDrawReadNone() override;
    void attachDepthCubeFace(unsigned int cube_map, int face) override;
    void clearColorDepthStencil(float r, float g, float b, float a) override;
    unsigned int newDepthCubeMap(int size) override;
    //// binding resource
    // void bindTexture(/*TextureData*/) override;
    // void bindVertexArray() override;
    // void bindBuffer(/*BufferData*/) override;

    RhiBuffer *newBuffer(RhiBuffer::Type type,
                         RhiBuffer::UsageFlag usage,
                         void *data,
                         int size) override;

    RhiVertexLayout *newVertexLayout(RhiBuffer *vbuffer, RhiBuffer *ibuffer) override;

    RhiTexture *newTexture(RhiTexture::Format format,
                           const Vec2 &pixelSize,
                           int sampleCount = 1,
                           RhiTexture::Flag flags = {},
                           unsigned char *data = nullptr) override;

    unsigned int newCubeTexture(int width, int height, const std::array<unsigned char*, 6>& datas) override;

    RhiFrameBuffer *newFrameBuffer(const RhiAttachment &colorAttachment, const Vec2 &pixelSize_, int sampleCount_ = 1) override;
};

#endif
