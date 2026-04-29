#ifndef Project_hpp
#define Project_hpp

#include <string>

struct Project {
	int schema_version = 1;
	std::string project_name = "XPYProject";

	// Runtime computed absolute path.
	std::string project_root_abs;
};

#endif // !Project_hpp
