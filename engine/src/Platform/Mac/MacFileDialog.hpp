#ifndef MacFileDialog_hpp
#define MacFileDialog_hpp

#include "Platform/FileDialog.hpp"

class MacFileDialog : public FileDialog {
public:
	MacFileDialog(Window* window) : FileDialog(window) {}

	std::string OpenFile(const char* filter) override;
	std::string SaveFile(const char* filter) override;
};

#endif // !MacFileDialog_hpp
