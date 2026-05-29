#include "Logical/Framework/World/SelectionManager.hpp"
#include "Logical/Framework/World/SceneObjectRegistry.hpp"

#include <algorithm>

SelectionManager::SelectionManager(SceneObjectRegistry& registry)
	: m_registry(registry)
{
}

std::vector<GObjectID> SelectionManager::getPickedObjectIDs() const
{
	std::vector<GObjectID> res(m_picked_objects.size());
	std::transform(m_picked_objects.begin(), m_picked_objects.end(), res.begin(),
		[](const std::shared_ptr<GObject>& obj) { return obj->ID(); });
	return res;
}

void SelectionManager::clear()
{
	m_picked_objects.clear();
}

void SelectionManager::removeObject(GObjectID id)
{
	m_picked_objects.erase(std::remove_if(m_picked_objects.begin(), m_picked_objects.end(),
		[id](const std::shared_ptr<GObject>& obj)
		{
			return obj && obj->ID() == id;
		}), m_picked_objects.end());
}

void SelectionManager::onPickedChanged(std::vector<GObjectID> added, std::vector<GObjectID> removed)
{
	m_picked_objects.erase(std::remove_if(m_picked_objects.begin(), m_picked_objects.end(),
		[&removed](const std::shared_ptr<GObject>& obj) {
			return std::find(removed.begin(), removed.end(), obj->ID()) != removed.end();
		}), m_picked_objects.end());

	for (const auto& obj : m_registry.getObjects()) {
		if (std::find(added.begin(), added.end(), obj->ID()) != added.end()) {
			m_picked_objects.push_back(obj);
			Logger::debug("SelectionManager::onPickedChanged(), added obj: {} {}", obj->ID().value(), obj->name());
		}
	}
}
