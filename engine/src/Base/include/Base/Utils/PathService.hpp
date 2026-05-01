#ifndef PathService_hpp
#define PathService_hpp

#include <algorithm>
#include <cctype>
#include <string>

namespace PathService {
	static inline bool isAbsolute(const std::string& path)
	{
		// Windows: "C:\\" or "C:/" ; Unix: "/"
		if (path.size() >= 2 && std::isalpha((unsigned char)path[0]) && path[1] == ':')
			return true;
		if (!path.empty() && (path[0] == '/' || path[0] == '\\'))
			return true;
		return false;
	}

	static inline std::string normalize(std::string path)
	{
		std::replace(path.begin(), path.end(), '\\', '/');
		// remove trailing '/' (except root like "C:/")
		while (path.size() > 1 && path.back() == '/') {
			if (path.size() == 3 && std::isalpha((unsigned char)path[0]) && path[1] == ':' && path[2] == '/')
				break;
			path.pop_back();
		}
		return path;
	}

	static inline std::string join(const std::string& base, const std::string& rel)
	{
		if (rel.empty())
			return normalize(base);
		if (isAbsolute(rel))
			return normalize(rel);

		std::string b = normalize(base);
		if (!b.empty() && b.back() != '/')
			b.push_back('/');
		return normalize(b + rel);
	}

	// Returns `path` relative to `base_dir` if possible; otherwise returns original `path`.
	static inline std::string tryMakeRelative(const std::string& base_dir, const std::string& path)
	{
		std::string base = normalize(base_dir);
		std::string p = normalize(path);

		if (base.empty())
			return p;
		if (!base.empty() && base.back() != '/')
			base.push_back('/');

		// case-insensitive compare on Windows-like drive paths
		auto starts_with_ci = [](const std::string& s, const std::string& prefix) {
			if (s.size() < prefix.size())
				return false;
			for (size_t i = 0; i < prefix.size(); i++) {
				char a = s[i];
				char b = prefix[i];
				if (a >= 'A' && a <= 'Z') a = char(a - 'A' + 'a');
				if (b >= 'A' && b <= 'Z') b = char(b - 'A' + 'a');
				if (a != b) return false;
			}
			return true;
		};

		if (starts_with_ci(p, base)) {
			return p.substr(base.size());
		}
		return p;
	}

    static std::string getDirectory(const std::string& filepath)
    {
        std::string path = normalize(filepath);
        const size_t pos = path.find_last_of('/');
        return (pos == std::string::npos) ? std::string() : path.substr(0, pos);
    }

    static inline std::string getFileName(const std::string& filepath) {
        return filepath.substr(filepath.find_last_of("/\\") + 1, filepath.find_last_of('.') - filepath.find_last_of("/\\") - 1);
    }

    static inline std::string getFileSuffix(const std::string& filepath) {
        return filepath.substr(filepath.find_last_of(".") + 1);
    }
}

#endif // !PathService_hpp
