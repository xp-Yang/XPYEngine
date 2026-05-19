#include "rhi_opengl.hpp"
#include "OpenGLFrameBuffer.hpp"
#include "OpenGLTexture.hpp"
#include "OpenGLBuffer.hpp"
#include "OpenGLVertexLayout.hpp"
#include "OpenGLRenderer.hpp"

#include <assert.h>
#include <stdexcept>

RhiOpenGL::RhiOpenGL()
{
    if (!glfwGetCurrentContext())
        throw std::runtime_error("OpenGL context must be created before RHI.");
	// 初始化GLAD，使其可以管理OpenGL函数指针
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		throw std::runtime_error("Failed to initialize GLAD");

	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LEQUAL);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
}

void RhiOpenGL::drawIndexed(unsigned int vao_id, size_t indices_count, size_t index_offset, int inst_amount)
{
	OpenGLRenderer::drawIndexed(vao_id, indices_count, index_offset, inst_amount);
}

void RhiOpenGL::drawTriangles(unsigned int vao_id, size_t array_count)
{
}

void RhiOpenGL::setViewport(int x, int y, int width, int height)
{
	glViewport(x, y, width, height);
}

void RhiOpenGL::setBlend(bool enable)
{
    if (enable)
        glEnable(GL_BLEND);
    else
        glDisable(GL_BLEND);
}

void RhiOpenGL::setDepthMask(bool enable)
{
	glDepthMask(enable ? GL_TRUE : GL_FALSE);
}

void RhiOpenGL::setFrontFaceCW(bool cw)
{
	glFrontFace(cw ? GL_CW : GL_CCW);
}

void RhiOpenGL::readPixelRGBA(unsigned int framebuffer, int x, int y, unsigned char out_rgba[4])
{
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, out_rgba);
}

RhiBuffer *RhiOpenGL::newBuffer(RhiBuffer::Type type, RhiBuffer::UsageFlag usage, void *data, int size)
{
	OpenGLBuffer *buffer = new OpenGLBuffer(type, usage, data, size);
	return buffer;
}

RhiVertexLayout *RhiOpenGL::newVertexLayout(RhiBuffer *vbuffer, RhiBuffer *ibuffer)
{
	OpenGLVertexLayout *vertex_layout = new OpenGLVertexLayout(vbuffer, ibuffer);
	return vertex_layout;
}

RhiTexture *RhiOpenGL::newTexture(RhiTexture::Format format, const Vec2 &pixelSize, int sampleCount, RhiTexture::Flag flags, unsigned char *data)
{
	OpenGLTexture *texture = new OpenGLTexture(format, pixelSize, sampleCount, flags, data);
	return texture;
}

RhiTexture* RhiOpenGL::newCubeTexture(RhiTexture::Format format, const Vec2& pixelSize, int sampleCount, RhiTexture::Flag flags, const std::array<unsigned char*, 6>& cube_datas)
{
    OpenGLTexture* texture = new OpenGLTexture(format, pixelSize, sampleCount, flags, cube_datas);
    return texture;
}

RhiFrameBuffer *RhiOpenGL::newFrameBuffer(const RhiAttachment &colorAttachment, const Vec2 &pixelSize_, int sampleCount_)
{
	OpenGLFrameBuffer *fb = new OpenGLFrameBuffer(colorAttachment, pixelSize_, sampleCount_);
	return fb;
}
