#ifndef SceneDirtyTracker_hpp
#define SceneDirtyTracker_hpp

#include "Logical/Framework/World/SceneDirty.hpp"
#include "Logical/Framework/World/IDAllocator.hpp"

#include <unordered_map>
#include <vector>

struct SceneChange {
	GObjectID object_id{};
	SceneDirtyFlags flags{ SceneDirtyFlagBit(SceneDirtyFlag::None) };
};

class SceneDirtyTracker {
public:
	void markDirty(GObjectID object_id, SceneDirtyFlags flags);
	void markFullResync();
	std::vector<SceneChange> consumeChanges();

private:
	std::unordered_map<GObjectID, SceneChange> m_pending_changes;
	bool m_full_resync{ true };
};

#endif // !SceneDirtyTracker_hpp
