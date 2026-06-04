#ifndef ImGuiSceneHierarchy_hpp
#define ImGuiSceneHierarchy_hpp

#include "Base/Common.hpp"
#include "Logical/Framework/World/IDAllocator.hpp"

#include <memory>
#include <vector>

namespace Snapshot {
class Transaction;
}

class GObject;
class ImGuiEditor;
struct RenderTextureResource;
class ImGuiSceneHierarchy {
public:
	ImGuiSceneHierarchy(ImGuiEditor* parent);
	~ImGuiSceneHierarchy();
	void render();

private:
	void beginObjectTransaction(GObject* object, const std::string& label);
	void endObjectTransaction(GObject* object);
	void commitImmediateObjectEdit(GObject* object, const std::string& label, const std::function<void()>& edit);

	std::unordered_map<std::string, std::function<void(std::string, const Meta::Instance&)>> m_widget_creator;
	std::unique_ptr<Snapshot::Transaction> m_active_transaction;
	GObjectID m_active_transaction_object_id{};
	GObject* m_current_inspected_object{ nullptr };
	std::vector<std::shared_ptr<RenderTextureResource>> m_texture_preview_frame_resources;
	ImGuiEditor* m_parent;
};

#endif
