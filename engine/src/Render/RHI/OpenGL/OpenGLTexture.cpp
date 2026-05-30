#include "OpenGLTexture.hpp"

OpenGLTexture::OpenGLTexture(Format format_, const Vec2 &pixelSize_, int sampleCount_, Flag flags_, unsigned char *data)
    : RhiTexture(format_, pixelSize_, sampleCount_, flags_, data)
{
}

OpenGLTexture::OpenGLTexture(Format format_, const Vec2& pixelSize_, int sampleCount_, Flag flags_, const std::array<unsigned char*, 6>& cube_datas)
    : RhiTexture(format_, pixelSize_, sampleCount_, flags_, cube_datas)
{
}

OpenGLTexture::~OpenGLTexture()
{
    destroy();
}

void OpenGLTexture::destroy()
{
	if (m_id != 0)
	{
		glDeleteTextures(1, &m_id);
		m_id = 0;
	}
}

struct TextureFormatDesc {
    GLenum internal_format{ GL_RGBA };
    GLenum format{ GL_RGBA };
    GLenum type{ GL_UNSIGNED_BYTE };
    int component_count{ 4 };
    int bytes_per_component{ 1 };
    bool is_depth{ false };
};

// Screen-space / offscreen render targets: clamp to avoid REPEAT bleed at edges
// when post-process shaders sample with texel offsets (bloom, FXAA, SSAO, etc.).
static void setRenderTargetWrapClamp2D()
{
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

static bool getTextureFormatDesc(RhiTexture::Format format, TextureFormatDesc& desc)
{
    switch (format)
    {
    case RhiTexture::Format::R8:
        desc = { GL_RED, GL_RED, GL_UNSIGNED_BYTE, 1, 1, false };
        return true;
    case RhiTexture::Format::RGB8:
        desc = { GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE, 3, 1, false };
        return true;
    case RhiTexture::Format::RGB16F:
        desc = { GL_RGB16F, GL_RGB, GL_FLOAT, 3, 4, false };
        return true;
    case RhiTexture::Format::RGBA8:
        desc = { GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, 4, 1, false };
        return true;
    case RhiTexture::Format::RGBA16F:
        desc = { GL_RGBA16F, GL_RGBA, GL_FLOAT, 4, 4, false };
        return true;
    case RhiTexture::Format::DEPTH:
        desc = { GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT, 1, 4, true };
        return true;
    case RhiTexture::Format::DEPTH24STENCIL8:
        desc = { GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 1, 4, true };
        return true;
    default:
        return false;
    }
}

bool OpenGLTexture::create()
{
    GL_HANDLE textureID;

    if (m_flags & RhiTexture::CubeMap)
    {
        assert(m_sampleCount == 1);

        TextureFormatDesc desc;
        if (!getTextureFormatDesc(m_format, desc))
        {
            assert(false);
            return false;
        }

        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

        const int width = (int)m_pixelSize.x;
        const int height = (int)m_pixelSize.y;
        for (int face = 0; face < 6; ++face)
        {
            const void* face_data = m_cube_datas[face];
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
                         0,
                         desc.internal_format,
                         width,
                         height,
                         0,
                         desc.format,
                         desc.type,
                         face_data);
        }

        const bool mipmapped = (m_flags & RhiTexture::MipMapped) != 0;
        const GLint min_filter = desc.is_depth ? GL_NEAREST : (mipmapped ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
        const GLint mag_filter = desc.is_depth ? GL_NEAREST : GL_LINEAR;
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, min_filter);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, mag_filter);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        if (mipmapped && !desc.is_depth)
            glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

        m_id = textureID;
        return true;
    }
    else
    {
        switch (m_format)
        {
        case RhiTexture::Format::R8:
        {
            if (m_sampleCount > 1)
            {
                glGenTextures(1, &textureID);
                glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textureID);
                glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_sampleCount, GL_R8, (int)m_pixelSize.x, (int)m_pixelSize.y, GL_TRUE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            }
            else
            {
                glGenTextures(1, &textureID);
                glBindTexture(GL_TEXTURE_2D, textureID);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, (int)m_pixelSize.x, (int)m_pixelSize.y, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                setRenderTargetWrapClamp2D();
            }
            break;
        }
        case RhiTexture::Format::RGB8:
        {
            if (m_sampleCount > 1)
            {
                glGenTextures(1, &textureID);
                glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textureID);
                glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_sampleCount, GL_RGB8, (int)m_pixelSize.x, (int)m_pixelSize.y, GL_TRUE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            }
            else
            {
                glGenTextures(1, &textureID);
                glBindTexture(GL_TEXTURE_2D, textureID);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, (int)m_pixelSize.x, (int)m_pixelSize.y, 0, GL_RGB, GL_FLOAT, NULL);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                setRenderTargetWrapClamp2D();
            }
            break;
        }
        case RhiTexture::Format::RGB16F:
        {
            if (m_sampleCount > 1)
            {
                glGenTextures(1, &textureID);
                glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textureID);
                glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_sampleCount, GL_RGB16F, (int)m_pixelSize.x, (int)m_pixelSize.y, GL_TRUE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            }
            else
            {
                glGenTextures(1, &textureID);
                glBindTexture(GL_TEXTURE_2D, textureID);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, (int)m_pixelSize.x, (int)m_pixelSize.y, 0, GL_RGB, GL_FLOAT, NULL);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                setRenderTargetWrapClamp2D();
            }
            break;
        }
        case RhiTexture::Format::RGBA8:
        {
            if (m_sampleCount > 1)
            {
                glGenTextures(1, &textureID);
                glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textureID);
                glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_sampleCount, GL_RGBA8, (int)m_pixelSize.x, (int)m_pixelSize.y, GL_TRUE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            }
            else
            {
                glGenTextures(1, &textureID);
                glBindTexture(GL_TEXTURE_2D, textureID);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (int)m_pixelSize.x, (int)m_pixelSize.y, 0, GL_RGBA, GL_FLOAT, NULL);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                setRenderTargetWrapClamp2D();
            }
            break;
        }
        case RhiTexture::Format::RGBA16F:
        {
            if (m_sampleCount > 1)
            {
                glGenTextures(1, &textureID);
                glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textureID);
                glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_sampleCount, GL_RGBA16F, (int)m_pixelSize.x, (int)m_pixelSize.y, GL_TRUE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            }
            else
            {
                glGenTextures(1, &textureID);
                glBindTexture(GL_TEXTURE_2D, textureID);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, (int)m_pixelSize.x, (int)m_pixelSize.y, 0, GL_RGBA, GL_FLOAT, NULL);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                setRenderTargetWrapClamp2D();
            }
            break;
        }
        case RhiTexture::Format::DEPTH24STENCIL8:
        {
            // 统一用texture附件，不使用rbo附件
            // glGenRenderbuffers(1, &rbo);
            // glBindRenderbuffer(GL_RENDERBUFFER, rbo);
            // glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_width, m_height);
            // glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

            if (m_sampleCount > 1)
            {
                glGenTextures(1, &textureID);
                glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textureID);
                glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_sampleCount, GL_DEPTH24_STENCIL8, (int)m_pixelSize.x, (int)m_pixelSize.y, GL_TRUE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            }
            else
            {
                glGenTextures(1, &textureID);
                glBindTexture(GL_TEXTURE_2D, textureID);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, (int)m_pixelSize.x, (int)m_pixelSize.y, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            }
            break;
        }
        case RhiTexture::Format::DEPTH:
        {
            if (m_sampleCount > 1)
            {
                glGenTextures(1, &textureID);
                glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textureID);
                glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_sampleCount, GL_DEPTH_COMPONENT, (int)m_pixelSize.x, (int)m_pixelSize.y, GL_TRUE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
                GLfloat borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
                glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
            }
            else
            {
                glGenTextures(1, &textureID);
                glBindTexture(GL_TEXTURE_2D, textureID);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, (int)m_pixelSize.x, (int)m_pixelSize.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
                GLfloat borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
                glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
            }
            // if depth only:
            // 不包含颜色缓冲的帧缓冲对象是不完整的，所以需要显式告诉OpenGL不使用任何颜色缓冲
            // glDrawBuffer(GL_NONE);
            // glReadBuffer(GL_NONE);
            break;
        }
        default:
            assert(false);
            break;
        }

        if (m_data)
        {
            GLenum internal_format;
            if (m_format == R8)
                internal_format = GL_RED;
            else if (m_format == RGB8)
                internal_format = GL_RGB;
            else if (m_format == RGB16F)
                internal_format = GL_RGB16F;
            else if (m_format == RGBA8)
                internal_format = GL_RGBA;
            else if (m_format == RGBA16F)
                internal_format = GL_RGBA16F;
            else
                assert(false);

            glBindTexture(GL_TEXTURE_2D, textureID);
            if (m_format == R8)
                glTexImage2D(GL_TEXTURE_2D, 0, internal_format, (int)m_pixelSize.x, (int)m_pixelSize.y, 0, GL_RED, GL_UNSIGNED_BYTE, m_data);
            else if (m_format == RGB8 || m_format == RGB16F)
                glTexImage2D(GL_TEXTURE_2D, 0, internal_format, (int)m_pixelSize.x, (int)m_pixelSize.y, 0, GL_RGB, GL_UNSIGNED_BYTE, m_data);
            else if (m_format == RGBA8 || m_format == RGBA16F)
                glTexImage2D(GL_TEXTURE_2D, 0, internal_format, (int)m_pixelSize.x, (int)m_pixelSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_data);
            else
                assert(false);
            glGenerateMipmap(GL_TEXTURE_2D);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }
    }

    m_id = textureID;
    return true;
}
