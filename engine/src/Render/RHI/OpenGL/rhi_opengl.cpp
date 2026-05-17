#include "rhi_opengl.hpp"
#include "OpenGLFrameBuffer.hpp"
#include "OpenGLTexture.hpp"
#include "OpenGLBuffer.hpp"
#include "OpenGLVertexLayout.hpp"
#include "OpenGLRenderer.hpp"

#include <stdexcept>

RhiOpenGL::RhiOpenGL()
{
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

unsigned int RhiOpenGL::newFramebufferHandle()
{
	unsigned int fbo = 0;
	glGenFramebuffers(1, &fbo);
	return fbo;
}

void RhiOpenGL::bindFramebuffer(unsigned int framebuffer)
{
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
}

void RhiOpenGL::bindDefaultFramebuffer()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RhiOpenGL::setFramebufferDrawReadNone()
{
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
}

void RhiOpenGL::attachDepthCubeFace(unsigned int cube_map, int face)
{
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, cube_map, 0);
}

void RhiOpenGL::clearColorDepthStencil(float r, float g, float b, float a)
{
	glClearColor(r, g, b, a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

unsigned int RhiOpenGL::newDepthCubeMap(int size)
{
	unsigned int texture = 0;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
	for (int i = 0; i < 6; ++i) {
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, size, size, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	return texture;
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

unsigned int RhiOpenGL::newCubeTexture(int width, int height, const std::array<unsigned char*, 6>& datas)
{
	unsigned int texture = 0;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
	for (unsigned int i = 0; i < datas.size(); ++i) {
		if (!datas[i]) {
			assert(false);
			continue;
		}
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, datas[i]);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	return texture;
}

RhiFrameBuffer *RhiOpenGL::newFrameBuffer(const RhiAttachment &colorAttachment, const Vec2 &pixelSize_, int sampleCount_)
{
	OpenGLFrameBuffer *fb = new OpenGLFrameBuffer(colorAttachment, pixelSize_, sampleCount_);
	return fb;
}
