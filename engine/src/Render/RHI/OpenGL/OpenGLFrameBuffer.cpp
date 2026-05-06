#include "OpenGLFrameBuffer.hpp"
#include "../rhi.hpp"
#include <assert.h>

OpenGLFrameBuffer::OpenGLFrameBuffer(const RhiAttachment &colorAttachment, const Vec2 &pixelSize_, int sampleCount_)
    : RhiFrameBuffer(colorAttachment, pixelSize_, sampleCount_)
{
}

void OpenGLFrameBuffer::destroyGPU()
{
	if (m_id != 0)
	{
		glDeleteFramebuffers(1, &m_id);
		m_id = 0;
	}
	for (auto &ca : m_colorAttachments)
		ca.release();
	m_depthAttachment.release();
	m_depthStencilAttachment.release();
}

bool OpenGLFrameBuffer::create()
{
    unsigned fbo_id;
    glGenFramebuffers(1, &fbo_id);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_id);

    int color_attachment_size = 0;
    for (int i = 0; i < m_colorAttachments.size(); i++)
    {
        RhiTexture *texture = m_colorAttachments[i].texture();
        if (texture)
        {
            color_attachment_size++;
            unsigned int textureID = texture->id();
            if (texture->sampleCount() > 1)
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D_MULTISAMPLE, textureID, 0);
            else
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, textureID, 0);
        }
    }
    RhiTexture *depth_stencil_texture = m_depthStencilAttachment.texture();
    if (depth_stencil_texture)
    {
        unsigned int depthStencilTextureID = depth_stencil_texture->id();
        if (depth_stencil_texture->sampleCount() > 1)
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE, depthStencilTextureID, 0);
        else
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depthStencilTextureID, 0);
    }

    RhiTexture *depth_texture = m_depthAttachment.texture();
    if (depth_texture)
    {
        unsigned int depthTextureID = depth_texture->id();
        if (depth_texture->sampleCount() > 1)
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE, depthTextureID, 0);
        else
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTextureID, 0);
    }

    if (color_attachment_size > 1)
    { // 有2个及以上
        unsigned int *attachments = new unsigned int[color_attachment_size];
        for (unsigned int i = 0; i < color_attachment_size; i++)
        {
            attachments[i] = GL_COLOR_ATTACHMENT0 + i;
        }
        glDrawBuffers(color_attachment_size, attachments);
        delete[] attachments;
        // glDrawBuffers函数并不是一个Draw Call,而是一个状态机参数设置的函数,它的作用是告诉OpenGL,把绘制output put填充到这些Attachment对应的Buffer里,所以这个函数在创建Framebuffer的时候就可以被调用了。
    }

    int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        printf("FBO Creation Failed! glError: %i\n", status);
        assert(false);
        return false;
    }

    m_id = fbo_id;

    return true;
}

// void OpenGLFrameBuffer::setSamples(int samples)
//{
//     // 如果创建的时候纹理对象不是multi-sample，就不能更改采样数
//     if (!isMultiSampled())
//         return;
//
//     m_samples = samples;
//     for (auto& attachment : m_attachments) {
//         if (attachment.getType() == AttachmentType::RGBA) {
//             glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, attachment.getMap());
//             glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_samples, GL_RGBA, m_width, m_height, GL_TRUE);
//         }
//         if (attachment.getType() == AttachmentType::DEPTH24STENCIL8) {
//             glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, attachment.getMap());
//             glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_samples, GL_DEPTH24_STENCIL8, m_width, m_height, GL_TRUE);
//         }
//         if (attachment.getType() == AttachmentType::DEPTH) {
//             glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, attachment.getMap());
//             glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_samples, GL_DEPTH_COMPONENT, m_width, m_height, GL_TRUE);
//         }
//     }
// }

void OpenGLFrameBuffer::bind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_id);

    // 默认视口变换填满framebuffer，(未填满的部分是clearColor)
    // frameBuffer 的大小总是整个窗口大小 (其实小点没啥关系，只是会糊)
    glViewport(0, 0, (int)m_pixelSize.x, (int)m_pixelSize.y);
}

void OpenGLFrameBuffer::blitTo(RhiFrameBuffer *dest, RhiTexture::Format format)
{
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_id);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dest->id());
    if (format == RhiTexture::Format::RGB16F || format == RhiTexture::Format::RGBA16F || format == RhiTexture::Format::RGB8 || format == RhiTexture::Format::RGBA8)
        glBlitFramebuffer(0, 0, (int)m_pixelSize.x, (int)m_pixelSize.y, 0, 0, (int)dest->pixelSize().x, (int)dest->pixelSize().y, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    if (format == RhiTexture::Format::DEPTH)
        glBlitFramebuffer(0, 0, (int)m_pixelSize.x, (int)m_pixelSize.y, 0, 0, (int)dest->pixelSize().x, (int)dest->pixelSize().y, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    if (format == RhiTexture::Format::DEPTH24STENCIL8)
        glBlitFramebuffer(0, 0, (int)m_pixelSize.x, (int)m_pixelSize.y, 0, 0, (int)dest->pixelSize().x, (int)dest->pixelSize().y, GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST);
}

void OpenGLFrameBuffer::unBind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OpenGLFrameBuffer::clear(Color4 clear_color)
{
    bind();
    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}
