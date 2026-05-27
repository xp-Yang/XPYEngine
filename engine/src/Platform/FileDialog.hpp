#ifndef FileDialog_hpp
#define FileDialog_hpp

#include <string>

class Window;
class FileDialog
{
public:
    FileDialog(Window* window) : m_window(window) {}

    virtual std::string OpenFile(const char* filter) = 0;
    virtual std::string SaveFile(const char* filter) = 0;
protected:
    FileDialog(const FileDialog&) = delete;
    FileDialog& operator=(const FileDialog&) = delete;
    virtual ~FileDialog() = default;

    Window* m_window;
};

#endif
