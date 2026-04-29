#ifndef Utils_hpp
#define Utils_hpp

namespace Utils
{

    inline std::string trim(const std::string &str, const std::string &spaces = " \t\r\n")
    {
        size_t a = str.find_first_not_of(spaces);
        size_t b = str.find_last_not_of(spaces) + 1;
        if (b <= a)
        {
            return std::string();
        }
        return str.substr(a, b - a);
    }

    inline bool starts_with(const std::string &str, const std::string &prefix)
    {
        if (prefix.size() > str.size())
        {
            return false;
        }
        return str.compare(0, prefix.size(), prefix) == 0;
    }
    inline bool starts_with(const std::string_view &str, const std::string_view &prefix)
    {
        if (prefix.size() > str.size())
        {
            return false;
        }
        return str.compare(0, prefix.size(), prefix) == 0;
    }

    inline bool iequals(const std::string &str1, const std::string &str2)
    {
        if (str1.size() != str2.size())
        {
            return false;
        }
        // 使用 std::equal 比较两个字符串的每个字符
        return std::equal(str1.begin(), str1.end(), str2.begin(),
                          [](char a, char b)
                          { return tolower(a) == tolower(b); });
    }
    inline bool iequals(const std::string_view &str1, const std::string_view &str2)
    {
        if (str1.size() != str2.size())
        {
            return false;
        }
        // 使用 std::equal 比较两个字符串的每个字符
        return std::equal(str1.begin(), str1.end(), str2.begin(),
                          [](char a, char b)
                          { return tolower(a) == tolower(b); });
    }

    inline std::vector<std::string> split(std::vector<std::string> &output, const std::string &str, char delimiter, bool compress)
    {
        if (compress)
        {
            size_t start = 0;
            size_t end = 0;
            while ((end = str.find(delimiter, start)) != std::string::npos)
            {
                if (end > start)
                { // 连续的delimiter将被跳过
                    output.push_back(str.substr(start, end - start));
                }
                start = end + 1;
            }

            // 处理最后一个子字符串
            if (start < str.length())
            {
                output.push_back(str.substr(start));
            }
        }
        else
        {
            std::stringstream ss(str);
            std::string token;
            while (std::getline(ss, token, delimiter))
            {
                output.push_back(token);
            }
        }

        return output;
    }

    inline std::string mat4ToStr(const Mat4 &mat, int indentation = 0)
    {
        std::string tab_str = "";
        for (int i = 0; i < indentation; i++)
        {
            tab_str += std::string("    ");
        }
        std::string formatter = tab_str + std::string("%.3f %.3f %.3f %.3f \n");

        std::string result;
        char buf[1024];
        sprintf_s(buf, formatter.c_str(), mat[0].x, mat[1].x, mat[2].x, mat[3].x);
        result += buf;
        sprintf_s(buf, formatter.c_str(), mat[0].y, mat[1].y, mat[2].y, mat[3].y);
        result += buf;
        sprintf_s(buf, formatter.c_str(), mat[0].z, mat[1].z, mat[2].z, mat[3].z);
        result += buf;
        sprintf_s(buf, formatter.c_str(), mat[0].w, mat[1].w, mat[2].w, mat[3].w);
        result += buf;
        // printf("%.3f %.3f %.3f %.3f \n", mat[0][0], mat[1][0], mat[2][0], mat[3][0]);
        // printf("%.3f %.3f %.3f %.3f \n", mat[0][1], mat[1][1], mat[2][1], mat[3][1]);
        // printf("%.3f %.3f %.3f %.3f \n", mat[0][2], mat[1][2], mat[2][2], mat[3][2]);
        // printf("%.3f %.3f %.3f %.3f \n", mat[0][3], mat[1][3], mat[2][3], mat[3][3]);
        sprintf_s(buf, "\n");
        result += buf;
        return result;
    }

    inline std::string vec3ToStr(const Vec3 &vec)
    {
        std::string result;
        char buf[1024];
        sprintf_s(buf, "%.3f %.3f %.3f \n", vec.x, vec.y, vec.z);
        result += buf;
        sprintf_s(buf, "\n");
        result += buf;
        return result;
    }

    inline int ColorToInt(Color4 color)
    {
        int r = ((int)(color.x * 255)) << 24;
        int g = ((int)(color.y * 255)) << 16;
        int b = ((int)(color.z * 255)) << 8;
        int a = ((int)(color.w * 255)) << 0;
        return r + g + b + a;
    }

    inline Color4 IntToColor(int color)
    {
        float r = ((color >> 24) & 0x000000FF) / 255.0f;
        float g = ((color >> 16) & 0x000000FF) / 255.0f;
        float b = ((color >> 8) & 0x000000FF) / 255.0f;
        float a = ((color >> 0) & 0x000000FF) / 255.0f;
        return Color4(r, g, b, a);
    }

}

#endif // !Utils_hpp
