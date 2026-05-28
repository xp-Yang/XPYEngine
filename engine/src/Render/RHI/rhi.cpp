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

void RhiShaderResourceBindings::clear()
{
    m_bindings.clear();
}

void RhiShaderResourceBindings::setBool(const std::string& name, bool value)
{
    Binding& binding = upsert(name, Bool);
    binding.int_value = value ? 1 : 0;
}

void RhiShaderResourceBindings::setInt(const std::string& name, int value)
{
    Binding& binding = upsert(name, Int);
    binding.int_value = value;
}

void RhiShaderResourceBindings::setFloat(const std::string& name, float value)
{
    Binding& binding = upsert(name, Float);
    binding.values[0] = value;
}

void RhiShaderResourceBindings::setFloat2(const std::string& name, float x, float y)
{
    Binding& binding = upsert(name, Float2);
    binding.values[0] = x;
    binding.values[1] = y;
}

void RhiShaderResourceBindings::setFloat2(const std::string& name, const Vec2& value)
{
    setFloat2(name, value.x, value.y);
}

void RhiShaderResourceBindings::setFloat3(const std::string& name, const Vec3& value)
{
    Binding& binding = upsert(name, Float3);
    binding.values[0] = value.x;
    binding.values[1] = value.y;
    binding.values[2] = value.z;
}

void RhiShaderResourceBindings::setFloat4(const std::string& name, float value1, float value2, float value3, float value4)
{
    Binding& binding = upsert(name, Float4);
    binding.values[0] = value1;
    binding.values[1] = value2;
    binding.values[2] = value3;
    binding.values[3] = value4;
}

void RhiShaderResourceBindings::setFloat4(const std::string& name, const Vec4& value)
{
    setFloat4(name, value.x, value.y, value.z, value.w);
}

void RhiShaderResourceBindings::setMatrix(const std::string& name, int count, const Mat4& mat_value)
{
    Binding& binding = upsert(name, Matrix);
    binding.matrices.assign(&mat_value, &mat_value + std::max(count, 0));
}

void RhiShaderResourceBindings::setTexture(const std::string& name, int texture_unit, RhiTexture* texture)
{
    Binding& binding = upsert(name, Texture2D);
    binding.texture_unit = texture_unit;
    binding.texture = texture;
}

void RhiShaderResourceBindings::setCubeTexture(const std::string& name, int texture_unit, RhiTexture* texture)
{
    Binding& binding = upsert(name, TextureCube);
    binding.texture_unit = texture_unit;
    binding.texture = texture;
}

RhiShaderResourceBindings::Binding& RhiShaderResourceBindings::upsert(const std::string& name, Type type)
{
    auto it = std::find_if(m_bindings.begin(), m_bindings.end(),
        [&name](const Binding& binding)
        {
            return binding.name == name;
        });
    if (it != m_bindings.end())
    {
        it->type = type;
        it->matrices.clear();
        return *it;
    }

    Binding binding;
    binding.name = name;
    binding.type = type;
    m_bindings.push_back(std::move(binding));
    return m_bindings.back();
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
