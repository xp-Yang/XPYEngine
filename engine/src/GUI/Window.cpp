#include "GUI/Window.hpp"

#include <GLFW/glfw3.h>
#include "GUI/FileDialog.hpp"
#ifdef _WIN32
#include "GUI/Platform/Windows/WindowsFileDialog.hpp"
#elif defined(__APPLE__)
#include "GUI/Platform/Mac/MacFileDialog.hpp"
#endif

#include <assert.h>
#include <stdexcept>
#include <utility>

namespace
{
#if !defined(_WIN32) && !defined(__APPLE__)
class NullFileDialog : public FileDialog
{
public:
	NullFileDialog(Window* window) : FileDialog(window) {}

	std::string OpenFile(const char* filter) override { return {}; }
	std::string SaveFile(const char* filter) override { return {}; }
};
#endif
}

static void drop_file_callback(GLFWwindow *window, int count, const char **paths)
{
	for (int i = 0; i < count; i++)
	{
		// std::string filepath = paths[i];
		// getSceneHierarchy()->loadModel(filepath);
	}
}

Window::Window(int width, int height)
	: m_width(width), m_height(height)
{
	glfwInit(); // 初始化GLFW;
#ifdef __APPLE__
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#endif
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	// glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);//不允许用户调整窗口的大小
	// glEnable(GL_MULTISAMPLE);
	// glfwWindowHint(GLFW_SAMPLES, 16);
#if NO_TITLE_BAR
	// 设置 offscreen context 的标志位，隐藏标题栏
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
#endif

	m_window = glfwCreateWindow(width, height, "XPYEngine", nullptr, nullptr);
	if (m_window == nullptr)
	{
		glfwTerminate();
		throw std::runtime_error("Failed to create GLFW window");
	}
	glfwMakeContextCurrent(m_window);
	glfwSwapInterval(0); // disable vsync

	glfwSetWindowUserPointer(m_window, this);

	glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow *window, int width, int height)
								   {
		if (width <= 0 || height <= 0)
			return;
		Window* thiz = (Window*)glfwGetWindowUserPointer(window);
		thiz->m_width = width;
		thiz->m_height = height; });

	glfwSetDropCallback(m_window, drop_file_callback);
}

void Window::shutdown()
{
	glfwDestroyWindow(m_window);
	glfwTerminate();
}

bool Window::shouldClose() const
{
	return glfwWindowShouldClose(m_window);
}

void Window::swapBuffer() const
{
	glfwPollEvents();		   // 检查触发事件（键盘输入、鼠标移动等）
	glfwSwapBuffers(m_window); // 交换颜色缓冲（它是一个储存着GLFW窗口每一个像素颜色的大缓冲），输出在屏幕上。
}

void *Window::getNativeWindowHandle() const
{
	return m_window;
}

int Window::getWidth() const
{
	return m_width;
}

int Window::getHeight() const
{
	return m_height;
}

float Window::getAspectRatio() const
{
	return (float)m_width / (float)m_height;
}

FileDialog *Window::createFileDialog()
{
#ifdef _WIN32
	WindowsFileDialog *file_dialog = new WindowsFileDialog(this);
#elif defined(__APPLE__)
	MacFileDialog *file_dialog = new MacFileDialog(this);
#else
	NullFileDialog *file_dialog = new NullFileDialog(this);
#endif
	return file_dialog;
}
