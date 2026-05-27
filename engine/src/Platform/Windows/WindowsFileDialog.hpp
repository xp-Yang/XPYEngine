#ifndef WindowsFileDialog_hpp
#define WindowsFileDialog_hpp

#include "Platform/FileDialog.hpp"

class WindowsFileDialog : public FileDialog {
public:
	WindowsFileDialog(Window* window) : FileDialog(window) {}

	std::string OpenFile(const char* filter) override;
	std::string SaveFile(const char* filter) override;
};


#endif // !FileDialog_hpp
