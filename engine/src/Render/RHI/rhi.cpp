#include "rhi.hpp"
#include "rhi_impl.hpp"
#include "OpenGL/rhi_opengl.hpp"

RhiTexture::RhiTexture(Format format_, const Vec2& pixelSize_, int sampleCount_, Flag flags_, unsigned char* data_)
	: m_format(format_)
	, m_pixelSize(pixelSize_)
	, m_sampleCount(sampleCount_)
	, m_flags(flags_)
	, m_data(data_)
{
}

RhiTexture::RhiTexture(Format format_, const Vec2& pixelSize_, int sampleCount_, Flag flags_, const std::array<unsigned char*, 6>& cube_datas_)
    : m_format(format_)
    , m_pixelSize(pixelSize_)
    , m_sampleCount(sampleCount_)
    , m_flags(flags_)
    , m_cube_datas(cube_datas_)
{
}

RhiFrameBuffer::RhiFrameBuffer(const RhiAttachment& colorAttachment, const Vec2& pixelSize_, int sampleCount_)
	: m_pixelSize(pixelSize_)
	, m_sampleCount(sampleCount_)
{
	m_colorAttachments[0] = colorAttachment;
}

RhiShaderStage::RhiShaderStage(Type type_, std::string source_, std::string debug_name_)
    : m_type(type_)
    , m_source(std::move(source_))
    , m_debug_name(std::move(debug_name_))
{
}

RhiAttachment::RhiAttachment(RhiTexture* texture, int layer, int level, bool owns_texture)
	: m_texture(texture)
    , m_layer(layer)
    , m_level(level)
    , m_owns_texture(owns_texture)
{
}

void RhiAttachment::release()
{
	if (m_texture && m_owns_texture)
	{
		m_texture->destroy();
		delete m_texture;
	}
    m_texture = nullptr;
}

void RhiTexture::destroy()
{
}

void RhiFrameBuffer::destroyGPU()
{
}

Rhi*& Rhi::get()
{
    static Rhi* rhi = nullptr;
    return rhi;
}

Rhi* Rhi::create()
{
	Rhi*& rhi = Rhi::get();
    if (!rhi) {
        rhi = new Rhi();
        rhi->m_impl = new RhiOpenGL();
    }
	return rhi;
}

void Rhi::drawIndexed(GL_HANDLE vao_id, size_t indices_count, size_t index_offset, int inst_amount)
{
	m_impl->drawIndexed(vao_id, indices_count, index_offset, inst_amount);
}

void Rhi::drawTriangles(GL_HANDLE vao_id, size_t array_count)
{
	m_impl->drawTriangles(vao_id, array_count);
}

void Rhi::setViewport(int x, int y, int width, int height)
{
	m_impl->setViewport(x, y, width, height);
}

void Rhi::setBlend(bool enable)
{
	m_impl->setBlend(enable);
}

void Rhi::setDepthMask(bool enable)
{
	m_impl->setDepthMask(enable);
}

void Rhi::setFrontFaceCW(bool cw)
{
	m_impl->setFrontFaceCW(cw);
}

void Rhi::readPixelRGBA(GL_HANDLE framebuffer, int x, int y, unsigned char out_rgba[4])
{
	m_impl->readPixelRGBA(framebuffer, x, y, out_rgba);
}

RhiBuffer* Rhi::newBuffer(RhiBuffer::Type type, RhiBuffer::UsageFlag usage, void* data, int size)
{
	return m_impl->newBuffer(type, usage, data, size);
}

RhiVertexLayout* Rhi::newVertexLayout(RhiBuffer* vbuffer, RhiBuffer* ibuffer)
{
	return m_impl->newVertexLayout(vbuffer, ibuffer);
}

RhiGraphicsPipeline* Rhi::newGraphicsPipeline()
{
    return m_impl->newGraphicsPipeline();
}

RhiCommandBuffer* Rhi::newCommandBuffer()
{
    return m_impl->newCommandBuffer();
}

RhiTexture* Rhi::newTexture(RhiTexture::Format format, const Vec2& pixelSize, int sampleCount, RhiTexture::Flag flags, unsigned char* data)
{
	return m_impl->newTexture(format, pixelSize, sampleCount, flags, data);
}

RhiTexture* Rhi::newCubeTexture(RhiTexture::Format format, const Vec2& pixelSize, int sampleCount, RhiTexture::Flag flags, const std::array<unsigned char*, 6>& cube_datas)
{
    return m_impl->newCubeTexture(format, pixelSize, sampleCount, flags, cube_datas);
}

RhiFrameBuffer* Rhi::newFrameBuffer(const RhiAttachment& colorAttachment, const Vec2& pixelSize_, int sampleCount_)
{
	return m_impl->newFrameBuffer(colorAttachment, pixelSize_, sampleCount_);
}

RhiBuffer::RhiBuffer(Type type_, UsageFlag usage_, void* data, int size_)
	: m_type(type_)
	, m_usage(usage_)
	, m_data(data)
	, m_size(size_)
{
}

RhiVertexLayout::RhiVertexLayout(RhiBuffer* vbuffer, RhiBuffer* ibuffer)
	: m_vbuffer(vbuffer)
	, m_ibuffer(ibuffer)
{
}
