#ifndef Utils_hpp
#define Utils_hpp

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "Base/Math/Matrix.hpp"

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
