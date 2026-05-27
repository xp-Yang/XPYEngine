#ifndef Window_hpp
#define Window_hpp

static inline constexpr float DEFAULT_WINDOW_WIDTH = 1920.0f;
static inline constexpr float DEFAULT_WINDOW_HEIGHT = 1080.0f;

struct GLFWwindow;
class FileDialog;
class Window {
public:
    Window(int width, int height);
    void shutdown();
    bool shouldClose() const;
    void swapBuffer() const;
    void* getNativeWindowHandle() const;
    int getWidth() const;
    int getHeight() const;
    float getAspectRatio() const;

    FileDialog* createFileDialog();

private:
    GLFWwindow* m_window{ nullptr };
    int m_width;
    int m_height;
};

#endif
