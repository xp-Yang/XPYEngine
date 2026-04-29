#include "Logical/Shader.hpp"

#include <fstream>
#include <sstream>

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
