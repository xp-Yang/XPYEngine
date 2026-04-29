#include "RenderShaderObject.hpp"

#include <Render/RHI/OpenGL/rhi_opengl.hpp>

#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <sstream>

#include "Base/Utils/Utils.hpp"
#include "Base/Logger/Logger.hpp"

RenderShaderObject::RenderShaderObject(const Shader &shader)
{
    const char *vShaderCode = shader.vsCode.c_str();
    const char *fShaderCode = shader.fsCode.c_str();
    const char *gShaderCode = shader.gsCode.c_str();

    bool has_geo_shader = !shader.gsCode.empty();

    // compile
    unsigned int vertex, fragment, geometry;
    int success;
    char infoLog[512];

    // 顶点着色器
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    // 打印编译错误（如果有的话）
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertex, 512, NULL, infoLog);
        Logger::error("SHADER::VERTEX::COMPILATION_FAILED\n{}\n", infoLog);
        assert(false);
    };

    if (has_geo_shader)
    {
        geometry = glCreateShader(GL_GEOMETRY_SHADER);
        glShaderSource(geometry, 1, &gShaderCode, NULL);
        glCompileShader(geometry);
        glGetShaderiv(geometry, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(geometry, 512, NULL, infoLog);
            Logger::error("SHADER::GEOMETRY::COMPILATION_FAILED\n{}\n", infoLog);
            assert(false);
        };
    }

    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragment, 512, NULL, infoLog);
        Logger::error("SHADER::FRAGMENT::COMPILATION_FAILED\n{}\n", infoLog);
        assert(false);
    };

    // 着色器程序
    m_id = glCreateProgram();
    glAttachShader(m_id, vertex);
    glAttachShader(m_id, fragment);
    if (has_geo_shader)
        glAttachShader(m_id, geometry);
    glLinkProgram(m_id);
    // 打印连接错误（如果有的话）
    glGetProgramiv(m_id, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(m_id, 512, NULL, infoLog);
        Logger::error("SHADER::PROGRAM::LINKING_FAILED\n{}\n", infoLog);
        assert(false);
    }
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    if (has_geo_shader)
        glDeleteShader(geometry);
}

void RenderShaderObject::start_using() const
{
    glUseProgram(m_id);
}

void RenderShaderObject::stop_using() const
{
    glUseProgram(0);
}

void RenderShaderObject::setBool(const std::string &name, bool value) const
{
    glUniform1i(glGetUniformLocation(m_id, name.c_str()), (int)value);
}
void RenderShaderObject::setInt(const std::string &name, int value) const
{
    glUniform1i(glGetUniformLocation(m_id, name.c_str()), value);
}
void RenderShaderObject::setFloat(const std::string &name, float value) const
{
    glUniform1f(glGetUniformLocation(m_id, name.c_str()), value);
}
void RenderShaderObject::setFloat4(const std::string &name, float value1, float value2, float value3, float value4) const
{
    GLint location = glGetUniformLocation(m_id, name.c_str());
    glUniform4f(location, value1, value2, value3, value4);
}
void RenderShaderObject::setFloat4(const std::string &name, const Vec4 &value) const
{
    GLint location = glGetUniformLocation(m_id, name.c_str());
    glUniform4f(location, value.x, value.y, value.z, value.w);
}
void RenderShaderObject::setFloat3(const std::string &name, const Vec3 &value) const
{
    GLint location = glGetUniformLocation(m_id, name.c_str());
    glUniform3f(location, value.x, value.y, value.z);
}
void RenderShaderObject::setMatrix(const std::string &name, int count, const Mat4 &mat_value) const
{
    GLuint transformLoc = glGetUniformLocation(m_id, name.c_str());
    glUniformMatrix4fv(transformLoc, count, GL_FALSE, /*glm::value_ptr(mat_value)*/ &(mat_value[0].x));
}
void RenderShaderObject::setTexture(const std::string &name, int texture_unit, unsigned int texture_id) const
{
    glActiveTexture(GL_TEXTURE0 + texture_unit);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    setInt(name, texture_unit);
    glActiveTexture(GL_TEXTURE0);
}

void RenderShaderObject::setCubeTexture(const std::string &name, int texture_unit, unsigned int texture_id) const
{
    glActiveTexture(GL_TEXTURE0 + texture_unit);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture_id);
    setInt(name, texture_unit);
    glActiveTexture(GL_TEXTURE0);
}

RenderShaderObject *RenderShaderObject::getShaderObject(const ShaderType &type)
{
    std::string resource_dir = RESOURCE_DIR;
    switch (type)
    {
    case ShaderType::PristineGridShader:
    {
        Shader shader = Shader{resource_dir + "/shader/pristineGrid.vs", resource_dir + "/shader/pristineGrid.fs"};
        return new RenderShaderObject(shader);
    }
    case ShaderType::GBufferShader:
    {
        Shader shader = Shader{resource_dir + "/shader/mesh.vs", resource_dir + "/shader/gBuffer_pbr.fs"};
        return new RenderShaderObject(shader);
    }
    case ShaderType::GBufferPhongShader:
    {
        Shader shader = Shader{resource_dir + "/shader/mesh.vs", resource_dir + "/shader/gBuffer_phong.fs"};
        return new RenderShaderObject(shader);
    }
    case ShaderType::DeferredLightingShader:
    {
        Shader shader = Shader{resource_dir + "/shader/screenQuad.vs", resource_dir + "/shader/deferredLighting_pbr.fs"};
        return new RenderShaderObject(shader);
    }
    case ShaderType::DeferredLightingPhongShader:
    {
        Shader shader = Shader{resource_dir + "/shader/screenQuad.vs", resource_dir + "/shader/deferredLighting_phong.fs"};
        return new RenderShaderObject(shader);
    }
    case ShaderType::BlinnPhongShader:
    {
        Shader shader = Shader{resource_dir + "/shader/mesh.vs", resource_dir + "/shader/fowardLighting_phong.fs"};
        return new RenderShaderObject(shader);
    }
    case ShaderType::PBRShader:
    {
        Shader shader = Shader{resource_dir + "/shader/mesh.vs", resource_dir + "/shader/fowardLighting_pbr.fs"};
        return new RenderShaderObject(shader);
    }
    case ShaderType::OneColorShader:
    {
        Shader shader = Shader{resource_dir + "/shader/mesh.vs", resource_dir + "/shader/oneColor.fs"};
        return new RenderShaderObject(shader);
    }
    case ShaderType::GcodeShader:
    {
        Shader shader = Shader{resource_dir + "/shader/gcode.vs", resource_dir + "/shader/gcode.fs"};
        return new RenderShaderObject(shader);
    }
    case ShaderType::GcodeInstancingShader:
    {
        Shader shader = Shader{resource_dir + "/shader/gcodeInstancing.vs", resource_dir + "/shader/gcodeInstancing.fs"};
        return new RenderShaderObject(shader);
    }
    case ShaderType::SkyboxShader:
    {
        Shader shader = Shader{resource_dir + "/shader/skybox.vs", resource_dir + "/shader/skybox.fs"};
        return new RenderShaderObject(shader);
    }
    case ShaderType::NormalShader:
    {
        Shader shader = Shader{resource_dir + "/shader/mesh.vs", resource_dir + "/shader/normal.fs", resource_dir + "/shader/normal.gs"};
        return new RenderShaderObject(shader);
    }
    case ShaderType::WireframeShader:
    {
        Shader shader = Shader{resource_dir + "/shader/wireframe2.vs", resource_dir + "/shader/wireframe2.fs"};
        return new RenderShaderObject(shader);
    }
    case ShaderType::CheckerboardShader:
    {
        Shader shader = Shader{resource_dir + "/shader/checkerboard.vs", resource_dir + "/shader/checkerboard.fs"};
        return new RenderShaderObject(shader);
    }
    case ShaderType::RayTracingShader:
    {
        Shader shader = Shader{resource_dir + "/shader/screenQuad.vs", resource_dir + "/shader/rayTracing.fs"};
        return new RenderShaderObject(shader);
    }
    case ShaderType::CombineShader:
    {
        Shader shader = Shader{resource_dir + "/shader/screenQuad.vs", resource_dir + "/shader/combine.fs"};
        return new RenderShaderObject(shader);
    }
    case ShaderType::CubeMapShader:
    {
        Shader shader = Shader{resource_dir + "/shader/mesh.vs", resource_dir + "/shader/cubeMap.fs"};
        // shader = Shader{ resource_dir + "/shader/cubeMap2.vs", resource_dir + "/shader/cubeMap2.fs", resource_dir + "/shader/cubeMap2.gs" };
        return new RenderShaderObject(shader);
    }
    case ShaderType::ExtractBrightShader:
    {
        Shader shader = Shader{resource_dir + "/shader/screenQuad.vs", resource_dir + "/shader/extractBright.fs"};
        return new RenderShaderObject(shader);
    }
    case ShaderType::GaussianBlur:
    {
        Shader shader = Shader{resource_dir + "/shader/screenQuad.vs", resource_dir + "/shader/gaussianBlur.fs"};
        return new RenderShaderObject(shader);
    }
    case ShaderType::OutlineShader:
    {
        Shader shader = Shader{resource_dir + "/shader/screenQuad.vs", resource_dir + "/shader/outline.fs"};
        return new RenderShaderObject(shader);
    }
    case ShaderType::InstancingShader:
    {
        Shader shader = Shader{resource_dir + "/shader/instancing.vs", resource_dir + "/shader/instancing.fs"};
        return new RenderShaderObject(shader);
    }
    case ShaderType::BillBoardShader:
    {
        Shader shader = Shader{resource_dir + "/shader/billBoard.vs", resource_dir + "/shader/billBoard.fs"};
        return new RenderShaderObject(shader);
    }
    case ShaderType::FXAAShader:
    {
        Shader shader = Shader{resource_dir + "/shader/screenQuad.vs", resource_dir + "/shader/fxaa.fs"};
        return new RenderShaderObject(shader);
    }
    default:
        break;
    }
    return nullptr;
}
