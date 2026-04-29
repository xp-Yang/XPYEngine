#include "WindowsFileDialog.hpp"

#include "GUI/Window.hpp"
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <commdlg.h> // Windows SDK

std::string WindowsFileDialog::OpenFile(const char* filter)
{
	// 打开搜索文件的窗口, 需要传入一个OPENFILENAMEA对象
	OPENFILENAMEA ofn;
	CHAR szFile[260] = { 0 };
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = glfwGetWin32Window((GLFWwindow*)(m_window->getNativeWindowHandle()));
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = filter;
	ofn.nFilterIndex = 1;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

	// 调用win32 api
	if (GetOpenFileNameA(&ofn) == TRUE)
		return ofn.lpstrFile;

	return {};
}

std::string WindowsFileDialog::SaveFile(const char* filter)
{
	OPENFILENAMEA ofn;
	CHAR szFile[260] = { 0 };
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = glfwGetWin32Window((GLFWwindow*)(m_window->getNativeWindowHandle()));
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = filter;
	ofn.nFilterIndex = 1;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

	// 调用win32 api
	// Sets the default extension by extracting it from the filter
	const char* ext = strchr(filter, '\0') + 1; // e.g. "*.proj"
	if (ext[0] == '*' && ext[1] == '.')
		ext += 2; // -> "proj"
	ofn.lpstrDefExt = ext;

	if (GetSaveFileNameA(&ofn) == TRUE)
		return ofn.lpstrFile;

	return {};
}
