#include "Engine.hpp"
#include "GUI/Window.hpp"
#include "Render/RenderSystem.hpp"
#include "GUI/Editor/ImGuiEditor.hpp"
#include "GUI/Editor/ImGuiInput.hpp"
#include "Logical/Animation/AnimationSystem.hpp"
#include "Logical/FrameWork/World/Scene.hpp"
#include "GlobalContext.hpp"
#include <utility>

Engine::Engine() {}

Engine::~Engine() {}

Engine& Engine::get()
{
	static Engine app;
	return app;
}

void Engine::run() {
	//Timer fps_timer;
	//fps_timer.start();
	while (!g_context.window->shouldClose()) {
		//double duration = fps_timer.duration();
		//if (duration >= MILLISECONDS_PER_FRAME) {
			//fps_timer.restart();
			g_context.gui_editor->beginFrame();
			{
				// logical
				g_context.gui_input->onUpdate();
				g_context.animation_system->onUpdate(g_context.scene);
				// render
				g_context.render_system->onUpdate(g_context.scene);
				// gui
				g_context.gui_editor->onUpdate();
				if (m_external_gui_callback)
					m_external_gui_callback();
			}
			g_context.gui_editor->endFrame();
			g_context.window->swapBuffer();
		//}
	}
	//fps_timer.stop();
}

void Engine::init()
{
}

void Engine::shutdown()
{
	g_context.window->shutdown();
}

void Engine::setExternalOpenFileHandler(std::function<bool(const std::string&)> handler)
{
	g_context.gui_editor->setExternalOpenFileHandler(std::move(handler));
}

void Engine::setExternalGuiCallback(ExternalGuiCallback callback)
{
	m_external_gui_callback = std::move(callback);
}

std::shared_ptr<Scene> Engine::Scene() const
{
	return g_context.scene;
}
