#include "OpenGLGraphicsPipeline.hpp"

#include <sstream>

namespace
{
GLenum toGlShaderStage(RhiShaderStage::Type type)
{
    switch (type)
    {
    case RhiShaderStage::Vertex: return GL_VERTEX_SHADER;
    case RhiShaderStage::Fragment: return GL_FRAGMENT_SHADER;
    case RhiShaderStage::Geometry: return GL_GEOMETRY_SHADER;
    default: return GL_VERTEX_SHADER;
    }
}

const char* shaderStageName(RhiShaderStage::Type type)
{
    switch (type)
    {
    case RhiShaderStage::Vertex: return "vertex";
    case RhiShaderStage::Fragment: return "fragment";
    case RhiShaderStage::Geometry: return "geometry";
    default: return "unknown";
    }
}

GLenum toGlTopology(RhiGraphicsPipeline::Topology topology)
{
    switch (topology)
    {
    case RhiGraphicsPipeline::Triangles: return GL_TRIANGLES;
    case RhiGraphicsPipeline::TriangleStrip: return GL_TRIANGLE_STRIP;
    case RhiGraphicsPipeline::TriangleFan: return GL_TRIANGLE_FAN;
    case RhiGraphicsPipeline::Lines: return GL_LINES;
    case RhiGraphicsPipeline::LineStrip: return GL_LINE_STRIP;
    case RhiGraphicsPipeline::Points: return GL_POINTS;
    default: return GL_TRIANGLES;
    }
}

GLenum toGlCullMode(RhiGraphicsPipeline::CullMode mode)
{
    switch (mode)
    {
    case RhiGraphicsPipeline::Front: return GL_FRONT;
    case RhiGraphicsPipeline::Back: return GL_BACK;
    case RhiGraphicsPipeline::None: return GL_BACK;
    default: return GL_BACK;
    }
}

GLenum toGlFrontFace(RhiGraphicsPipeline::FrontFace face)
{
    return face == RhiGraphicsPipeline::CW ? GL_CW : GL_CCW;
}

GLenum toGlBlendFactor(RhiGraphicsPipeline::BlendFactor factor)
{
    switch (factor)
    {
    case RhiGraphicsPipeline::Zero: return GL_ZERO;
    case RhiGraphicsPipeline::One: return GL_ONE;
    case RhiGraphicsPipeline::SrcColor: return GL_SRC_COLOR;
    case RhiGraphicsPipeline::OneMinusSrcColor: return GL_ONE_MINUS_SRC_COLOR;
    case RhiGraphicsPipeline::DstColor: return GL_DST_COLOR;
    case RhiGraphicsPipeline::OneMinusDstColor: return GL_ONE_MINUS_DST_COLOR;
    case RhiGraphicsPipeline::SrcAlpha: return GL_SRC_ALPHA;
    case RhiGraphicsPipeline::OneMinusSrcAlpha: return GL_ONE_MINUS_SRC_ALPHA;
    case RhiGraphicsPipeline::DstAlpha: return GL_DST_ALPHA;
    case RhiGraphicsPipeline::OneMinusDstAlpha: return GL_ONE_MINUS_DST_ALPHA;
    case RhiGraphicsPipeline::ConstantColor: return GL_CONSTANT_COLOR;
    case RhiGraphicsPipeline::OneMinusConstantColor: return GL_ONE_MINUS_CONSTANT_COLOR;
    case RhiGraphicsPipeline::ConstantAlpha: return GL_CONSTANT_ALPHA;
    case RhiGraphicsPipeline::OneMinusConstantAlpha: return GL_ONE_MINUS_CONSTANT_ALPHA;
    case RhiGraphicsPipeline::SrcAlphaSaturate: return GL_SRC_ALPHA_SATURATE;
    default: return GL_ONE;
    }
}

GLenum toGlBlendOp(RhiGraphicsPipeline::BlendOp op)
{
    switch (op)
    {
    case RhiGraphicsPipeline::Add: return GL_FUNC_ADD;
    case RhiGraphicsPipeline::Subtract: return GL_FUNC_SUBTRACT;
    case RhiGraphicsPipeline::ReverseSubtract: return GL_FUNC_REVERSE_SUBTRACT;
    case RhiGraphicsPipeline::Min: return GL_MIN;
    case RhiGraphicsPipeline::Max: return GL_MAX;
    default: return GL_FUNC_ADD;
    }
}

GLenum toGlCompareOp(RhiGraphicsPipeline::CompareOp op)
{
    switch (op)
    {
    case RhiGraphicsPipeline::Never: return GL_NEVER;
    case RhiGraphicsPipeline::Less: return GL_LESS;
    case RhiGraphicsPipeline::Equal: return GL_EQUAL;
    case RhiGraphicsPipeline::LessOrEqual: return GL_LEQUAL;
    case RhiGraphicsPipeline::Greater: return GL_GREATER;
    case RhiGraphicsPipeline::NotEqual: return GL_NOTEQUAL;
    case RhiGraphicsPipeline::GreaterOrEqual: return GL_GEQUAL;
    case RhiGraphicsPipeline::Always: return GL_ALWAYS;
    default: return GL_ALWAYS;
    }
}

GLenum toGlStencilOp(RhiGraphicsPipeline::StencilOp op)
{
    switch (op)
    {
    case RhiGraphicsPipeline::StencilZero: return GL_ZERO;
    case RhiGraphicsPipeline::Keep: return GL_KEEP;
    case RhiGraphicsPipeline::Replace: return GL_REPLACE;
    case RhiGraphicsPipeline::IncrementAndClamp: return GL_INCR;
    case RhiGraphicsPipeline::DecrementAndClamp: return GL_DECR;
    case RhiGraphicsPipeline::Invert: return GL_INVERT;
    case RhiGraphicsPipeline::IncrementAndWrap: return GL_INCR_WRAP;
    case RhiGraphicsPipeline::DecrementAndWrap: return GL_DECR_WRAP;
    default: return GL_KEEP;
    }
}

bool compileShader(GL_HANDLE shader, const RhiShaderStage& stage)
{
    const char* source = stage.source().c_str();
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success == GL_TRUE)
        return true;

    char info_log[1024] = {};
    glGetShaderInfoLog(shader, sizeof(info_log), nullptr, info_log);
    Logger::error("RHI shader compilation failed [{}:{}]\n{}",
                  shaderStageName(stage.type()),
                  stage.debugName(),
                  info_log);
    return false;
}
}

OpenGLGraphicsPipeline::~OpenGLGraphicsPipeline()
{
    destroy();
}

void OpenGLGraphicsPipeline::destroy()
{
    if (m_program != 0)
    {
        m_uniform_cache.clear();
        glDeleteProgram(m_program);
        m_program = 0;
    }
}

GLint OpenGLGraphicsPipeline::uniformLocation(const std::string& name)
{
    auto it = m_uniform_cache.find(name);
    if (it != m_uniform_cache.end())
        return it->second;

    const GLint loc = glGetUniformLocation(m_program, name.c_str());
    m_uniform_cache.emplace(name, loc);
    return loc;
}

bool OpenGLGraphicsPipeline::create()
{
    destroy();

    if (m_shaderStages.empty())
    {
        Logger::error("RHI graphics pipeline creation failed: no shader stages were provided.");
        return false;
    }

    bool has_vertex_stage = false;
    std::vector<GL_HANDLE> shaders;
    shaders.reserve(m_shaderStages.size());

    // OpenGL 的“pipeline”核心是 shader program。这里在 create() 中完成编译和
    // 链接，之后 setGraphicsPipeline() 只需要 glUseProgram + 应用固定状态。
    m_program = glCreateProgram();
    for (const RhiShaderStage& stage : m_shaderStages)
    {
        has_vertex_stage = has_vertex_stage || stage.type() == RhiShaderStage::Vertex;
        const GL_HANDLE shader = glCreateShader(toGlShaderStage(stage.type()));
        if (!compileShader(shader, stage))
        {
            glDeleteShader(shader);
            destroy();
            return false;
        }
        glAttachShader(m_program, shader);
        shaders.push_back(shader);
    }

    if (!has_vertex_stage)
    {
        Logger::error("RHI graphics pipeline creation failed: vertex shader stage is required.");
        for (GL_HANDLE shader : shaders)
            glDeleteShader(shader);
        destroy();
        return false;
    }

    glLinkProgram(m_program);

    GLint success = GL_FALSE;
    glGetProgramiv(m_program, GL_LINK_STATUS, &success);
    for (GL_HANDLE shader : shaders)
    {
        glDetachShader(m_program, shader);
        glDeleteShader(shader);
    }

    if (success != GL_TRUE)
    {
        char info_log[1024] = {};
        glGetProgramInfoLog(m_program, sizeof(info_log), nullptr, info_log);
        Logger::error("RHI graphics pipeline link failed:\n{}", info_log);
        destroy();
        return false;
    }

    m_draw_mode = toGlTopology(m_topology);
    return true;
}

void OpenGLGraphicsPipeline::bind()
{
    glUseProgram(m_program);

    if (m_cullMode == RhiGraphicsPipeline::None)
    {
        glDisable(GL_CULL_FACE);
    }
    else
    {
        glEnable(GL_CULL_FACE);
        glCullFace(toGlCullMode(m_cullMode));
    }
    glFrontFace(toGlFrontFace(m_frontFace));

    if (m_targetBlends.empty())
    {
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDisable(GL_BLEND);
    }
    else
    {
        const TargetBlend& blend = m_targetBlends.front();
        glColorMask((blend.colorWrite & R) != 0,
                    (blend.colorWrite & G) != 0,
                    (blend.colorWrite & B) != 0,
                    (blend.colorWrite & A) != 0);

        if (blend.enable)
        {
            glEnable(GL_BLEND);
            glBlendFuncSeparate(toGlBlendFactor(blend.srcColor),
                                toGlBlendFactor(blend.dstColor),
                                toGlBlendFactor(blend.srcAlpha),
                                toGlBlendFactor(blend.dstAlpha));
            glBlendEquationSeparate(toGlBlendOp(blend.opColor), toGlBlendOp(blend.opAlpha));
        }
        else
        {
            glDisable(GL_BLEND);
        }
    }

    if (m_depthTest)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);
    glDepthMask(m_depthWrite ? GL_TRUE : GL_FALSE);
    glDepthFunc(toGlCompareOp(m_depthOp));

    if (m_stencilTest)
    {
        glEnable(GL_STENCIL_TEST);
        applyStencilRef(m_stencil_ref);
        glStencilOpSeparate(GL_FRONT,
                            toGlStencilOp(m_stencilFront.failOp),
                            toGlStencilOp(m_stencilFront.depthFailOp),
                            toGlStencilOp(m_stencilFront.passOp));
        glStencilOpSeparate(GL_BACK,
                            toGlStencilOp(m_stencilBack.failOp),
                            toGlStencilOp(m_stencilBack.depthFailOp),
                            toGlStencilOp(m_stencilBack.passOp));
        glStencilMask(m_stencilWriteMask);
    }
    else
    {
        glDisable(GL_STENCIL_TEST);
    }

    if ((m_flags & UsesScissor) == 0)
        glDisable(GL_SCISSOR_TEST);

    if (m_depthBias != 0 || m_slopeScaledDepthBias != 0.0f)
    {
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(m_slopeScaledDepthBias, static_cast<float>(m_depthBias));
    }
    else
    {
        glDisable(GL_POLYGON_OFFSET_FILL);
    }

    if (m_topology == Lines || m_topology == LineStrip)
        glLineWidth(m_lineWidth);
}

void OpenGLGraphicsPipeline::applyStencilRef(int ref_value)
{
    m_stencil_ref = ref_value;
    if (!m_stencilTest)
        return;

    glStencilFuncSeparate(GL_FRONT, toGlCompareOp(m_stencilFront.compareOp), ref_value, m_stencilReadMask);
    glStencilFuncSeparate(GL_BACK, toGlCompareOp(m_stencilBack.compareOp), ref_value, m_stencilReadMask);
}
