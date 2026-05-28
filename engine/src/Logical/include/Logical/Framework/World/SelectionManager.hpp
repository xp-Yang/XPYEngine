#ifndef SelectionManager_hpp
#define SelectionManager_hpp

#include "Logical/Framework/Object/GObject.hpp"

#include <vector>
#include <memory>

class SceneObjectRegistry;

class SelectionManager {
public:
	explicit SelectionManager(SceneObjectRegistry& registry);

	const std::vector<std::shared_ptr<GObject>>& getPickedObjects() const { return m_picked_objects; }
	std::vector<GObjectID> getPickedObjectIDs() const;
	void clear();
	void removeObject(GObjectID id);

public slots:
	void onPickedChanged(std::vector<GObjectID> added, std::vector<GObjectID> removed);

private:
	SceneObjectRegistry& m_registry;
	std::vector<std::shared_ptr<GObject>> m_picked_objects;
};

#endif // !SelectionManager_hpp
