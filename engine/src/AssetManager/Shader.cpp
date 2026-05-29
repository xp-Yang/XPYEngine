#include "AssetManager/Shader.hpp"

#include <fstream>
#include <memory>
#include <sstream>
#include <unordered_map>

#include "Base/Utils/Utils.hpp"
#include "Base/Logger/Logger.hpp"

class ShaderParser
{
public:
    struct GlslLine
    {
        enum Tag
        {
            Include,
            Code,
        };
        Tag tag;
        std::string line;
        GlslLine(Tag tag_, const std::string &line_) : tag(tag_), line(line_) {}
    };

public:
    ShaderParser(const std::string &filepath)
    {
        Logger::info("parsing the shader file {}", filepath);
        load_file(filepath);
    }

    std::string getProcessedSourceCode()
    {
        std::string res;
        for (const auto &line : m_lines)
        {
            if (line.tag == GlslLine::Code)
            {
                res += (line.line + "\n");
            }
        }
        return res;
    }

protected:
    bool load_file(const std::string &filepath)
    {
        std::string directory = filepath.substr(0, filepath.find_last_of("/\\"));

        std::stringstream buffer;
        try
        {
            std::ifstream file_stream;
            file_stream.open(filepath);
            if (file_stream.is_open())
            {
                buffer << file_stream.rdbuf();
                file_stream.close();
            }
            else
            {
                assert(false);
                return false;
            }
        }
        catch (...)
        {
            Logger::error("SHADER FILE NOT SUCCESFULLY READ");
            assert(false);
            return false;
        }

        while (!buffer.eof())
        {
            std::string line;
            std::getline(buffer, line);

            size_t pos = line.find_first_not_of(" ");
            if (pos == std::string::npos)
            {
                m_lines.push_back(GlslLine(GlslLine::Code, line));
                continue;
            }

            if (line[pos] != '#')
            {
                m_lines.push_back(GlslLine(GlslLine::Code, line));
                continue;
            }

            // the line is started by '#'
            std::stringstream stm(line);
            std::string tag;
            stm >> tag;

            if (tag == "#include")
            {
                stm >> tag;
                tag = tag.substr(0, tag.find("//")); // 过滤注释
                tag = Utils::trim(tag, " \t\r\n\"<>");

                // 加载 include 文件
                std::string include_filepath = directory + '/' + tag;
                if (!this->load_file(include_filepath))
                {
                    assert(false);
                    return false;
                }

                m_lines.push_back(GlslLine(GlslLine::Include, std::string("// #include ") + include_filepath));
            }
            else
            {
                m_lines.push_back(GlslLine(GlslLine::Code, line));
            }
        }

        return true;
    }

private:
    std::vector<GlslLine> m_lines;
};

Shader::Shader(const std::string &vs_filepath, const std::string &fs_filepath)
{
    if (vs_filepath.empty())
    {
        Logger::error("vertex shader is empty!");
        assert(false);
    }
    if (fs_filepath.empty())
    {
        Logger::error("fragment shader is empty!");
        assert(false);
    }

    // read the source code and process file include
    ShaderParser v_parser(vs_filepath);
    vsCode = v_parser.getProcessedSourceCode();

    ShaderParser f_parser(fs_filepath);
    fsCode = f_parser.getProcessedSourceCode();
}

Shader::Shader(const std::string &vs_filepath, const std::string &fs_filepath, const std::string &gs_filepath)
{
    if (vs_filepath.empty())
    {
        Logger::error("vertex shader is empty!");
        assert(false);
    }
    if (fs_filepath.empty())
    {
        Logger::error("fragment shader is empty!");
        assert(false);
    }
    if (gs_filepath.empty())
    {
        Logger::error("geometry shader is empty!");
        assert(false);
    }

    // read the source code and process file include
    ShaderParser v_parser(vs_filepath);
    vsCode = v_parser.getProcessedSourceCode();

    ShaderParser f_parser(fs_filepath);
    fsCode = f_parser.getProcessedSourceCode();

    ShaderParser g_parser(gs_filepath);
    gsCode = g_parser.getProcessedSourceCode();
}

Shader Shader::create(const ShaderType &type)
{
    const std::string asset_dir = ASSET_DIR;
    switch (type)
    {
    case ShaderType::PristineGridShader:
        return Shader{asset_dir + "/shader/pristineGrid.vs", asset_dir + "/shader/pristineGrid.fs"};
    case ShaderType::GBufferShader:
        return Shader{asset_dir + "/shader/mesh.vs", asset_dir + "/shader/gBuffer_pbr.fs"};
    case ShaderType::GBufferPhongShader:
        return Shader{asset_dir + "/shader/mesh.vs", asset_dir + "/shader/gBuffer_phong.fs"};
    case ShaderType::DeferredLightingShader:
        return Shader{asset_dir + "/shader/screenQuad.vs", asset_dir + "/shader/deferredLighting_pbr.fs"};
    case ShaderType::DeferredLightingPhongShader:
        return Shader{asset_dir + "/shader/screenQuad.vs", asset_dir + "/shader/deferredLighting_phong.fs"};
    case ShaderType::BlinnPhongShader:
        return Shader{asset_dir + "/shader/mesh.vs", asset_dir + "/shader/fowardLighting_phong.fs"};
    case ShaderType::PBRShader:
        return Shader{asset_dir + "/shader/mesh.vs", asset_dir + "/shader/fowardLighting_pbr.fs"};
    case ShaderType::SingleColorShader:
        return Shader{asset_dir + "/shader/mesh.vs", asset_dir + "/shader/singleColor.fs"};
    case ShaderType::SkyboxShader:
        return Shader{asset_dir + "/shader/skybox.vs", asset_dir + "/shader/skybox.fs"};
    case ShaderType::NormalShader:
        return Shader{asset_dir + "/shader/mesh.vs", asset_dir + "/shader/normal.fs", asset_dir + "/shader/normal.gs"};
    case ShaderType::WireframeShader:
        return Shader{asset_dir + "/shader/wireframe2.vs", asset_dir + "/shader/wireframe2.fs"};
    case ShaderType::CheckerboardShader:
        return Shader{asset_dir + "/shader/checkerboard.vs", asset_dir + "/shader/checkerboard.fs"};
    case ShaderType::RayTracingShader:
        return Shader{asset_dir + "/shader/screenQuad.vs", asset_dir + "/shader/rayTracing.fs"};
    case ShaderType::BloomShader:
        return Shader{asset_dir + "/shader/screenQuad.vs", asset_dir + "/shader/bloom.fs"};
    case ShaderType::BloomDownsampleShader:
        return Shader{asset_dir + "/shader/screenQuad.vs", asset_dir + "/shader/bloomDownsample.fs"};
    case ShaderType::BloomUpsampleShader:
        return Shader{asset_dir + "/shader/screenQuad.vs", asset_dir + "/shader/bloomUpsample.fs"};
    case ShaderType::BloomCompositeShader:
        return Shader{asset_dir + "/shader/screenQuad.vs", asset_dir + "/shader/bloom.fs"};
    case ShaderType::CubeMapShader:
        return Shader{asset_dir + "/shader/mesh.vs", asset_dir + "/shader/cubeMap.fs"};
    case ShaderType::ExtractBrightShader:
        return Shader{asset_dir + "/shader/screenQuad.vs", asset_dir + "/shader/extractBright.fs"};
    case ShaderType::GaussianBlur:
        return Shader{asset_dir + "/shader/screenQuad.vs", asset_dir + "/shader/gaussianBlur.fs"};
    case ShaderType::OutlineShader:
        return Shader{asset_dir + "/shader/screenQuad.vs", asset_dir + "/shader/outline.fs"};
    case ShaderType::InstancingShader:
        return Shader{asset_dir + "/shader/instancing.vs", asset_dir + "/shader/instancing.fs"};
    case ShaderType::BillBoardShader:
        return Shader{asset_dir + "/shader/billBoard.vs", asset_dir + "/shader/billBoard.fs"};
    case ShaderType::FXAAShader:
        return Shader{asset_dir + "/shader/screenQuad.vs", asset_dir + "/shader/fxaa.fs"};
    case ShaderType::ToneMappingShader:
        return Shader{asset_dir + "/shader/screenQuad.vs", asset_dir + "/shader/tonemapping.fs"};
    case ShaderType::DebugTexturePreviewShader:
        return Shader{asset_dir + "/shader/debugTexturePreview.vs", asset_dir + "/shader/debugTexturePreview.fs"};
    case ShaderType::TransparentShader:
        return Shader{asset_dir + "/shader/mesh.vs", asset_dir + "/shader/transparent.fs"};
    default:
        Logger::error("Unknown shader type.");
        assert(false);
        return Shader{asset_dir + "/shader/mesh.vs", asset_dir + "/shader/singleColor.fs"};
    }
}

const Shader &Shader::get(const ShaderType &type)
{
    static std::unordered_map<ShaderType, std::unique_ptr<Shader>> shader_cache;

    auto it = shader_cache.find(type);
    if (it != shader_cache.end())
        return *it->second;

    auto shader = std::make_unique<Shader>(Shader::create(type));
    const Shader &shader_ref = *shader;
    shader_cache.emplace(type, std::move(shader));
    return shader_ref;
}
