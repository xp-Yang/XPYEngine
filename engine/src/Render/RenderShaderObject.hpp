#ifndef RenderShaderObject_hpp
#define RenderShaderObject_hpp

#include "Base/Math/Math.hpp"
#include "Logical/Shader.hpp"
#include "Render/RHI/rhi.hpp"

struct RenderPipelineState {
    bool blend{ false };
    bool depthTest{ true };
    bool depthWrite{ true };
    RhiGraphicsPipeline::CullMode cullMode{ RhiGraphicsPipeline::Back };
    RhiGraphicsPipeline::FrontFace frontFace{ RhiGraphicsPipeline::CCW };
    RhiGraphicsPipeline::Topology topology{ RhiGraphicsPipeline::Triangles };

    bool operator==(const RenderPipelineState& rhs) const
    {
        return blend == rhs.blend &&
            depthTest == rhs.depthTest &&
            depthWrite == rhs.depthWrite &&
            cullMode == rhs.cullMode &&
            frontFace == rhs.frontFace &&
            topology == rhs.topology;
    }
};

class RenderShaderObject {
public:
    static RenderShaderObject* getShaderObject(const ShaderType& type);

    RenderShaderObject(const Shader& shader);
    ~RenderShaderObject();

    RhiGraphicsPipeline* graphicsPipeline(const RenderPipelineState& state = RenderPipelineState());

    void start_using() const;
    void stop_using() const;
    // uniform setter
    void setBool(const std::string& name, bool value) const;
    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
    void setFloat4(const std::string& name, float value1, float value2, float value3, float value4) const;
    void setFloat4(const std::string& name, const Vec4& value) const;
    void setFloat3(const std::string& name, const Vec3& value) const;
    void setMatrix(const std::string& name, int count, const Mat4& mat_value) const;
    void setTexture(const std::string& name, int texture_unit, GL_HANDLE texture_id) const;
    void setCubeTexture(const std::string& name, int texture_unit, GL_HANDLE texture_id) const;

private:
    struct PipelineVariant {
        RenderPipelineState state;
        RhiGraphicsPipeline* pipeline{ nullptr };
    };

    GL_HANDLE m_id;
    std::string m_vertexCode;
    std::string m_fragmentCode;
    std::string m_geometryCode;
    std::vector<PipelineVariant> m_pipeline_variants;
};

#endif // !RenderShaderObject_hpp
