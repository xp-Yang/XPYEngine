#ifndef Engine_hpp
#define Engine_hpp

#include <string>
#include <memory>
#include <functional>

static inline const std::string RESOURCE_DIRECTORY = RESOURCE_DIR;

static inline int    FRAMES_PER_SECOND = 720;
static inline double MILLISECONDS_PER_FRAME = 1000. / FRAMES_PER_SECOND;

class Scene;
class Engine {
public:
	using ExternalGuiCallback = std::function<void()>;
	~Engine();
	static Engine& get();
	void run();
	void init();
	void shutdown();
	void setExternalOpenFileHandler(std::function<bool(const std::string&)> handler);
	void setExternalGuiCallback(ExternalGuiCallback callback);

	std::shared_ptr<Scene> Scene() const;

private:
	Engine();
	ExternalGuiCallback m_external_gui_callback;
};

#endif
