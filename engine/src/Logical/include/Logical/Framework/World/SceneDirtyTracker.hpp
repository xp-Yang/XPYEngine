#ifndef SceneDirtyTracker_hpp
#define SceneDirtyTracker_hpp

#include "Logical/Framework/World/SceneDirty.hpp"
#include "Logical/Framework/World/IDAllocator.hpp"

#include <unordered_map>
#include <vector>

struct SceneChange {
	int object_id{ 0 };
	SceneDirtyFlags flags{ SceneDirtyFlagBit(SceneDirtyFlag::None) };
};

class SceneDirtyTracker {
public:
	void markDirty(int object_id, SceneDirtyFlags flags);
	void markFullResync();
	std::vector<SceneChange> consumeChanges();

private:
	std::unordered_map<int, SceneChange> m_pending_changes;
	bool m_full_resync{ true };
};

#endif // !SceneDirtyTracker_hpp
