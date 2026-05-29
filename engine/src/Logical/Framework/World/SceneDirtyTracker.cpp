#include "Logical/Framework/World/SceneDirtyTracker.hpp"

void SceneDirtyTracker::markDirty(GObjectID object_id, SceneDirtyFlags flags)
{
	if (HasSceneDirtyFlag(flags, SceneDirtyFlag::FullResync))
	{
		markFullResync();
		return;
	}
	if (flags == SceneDirtyFlagBit(SceneDirtyFlag::None))
		return;

	auto& change = m_pending_changes[object_id];
	change.object_id = object_id;
	change.flags |= flags;
}

void SceneDirtyTracker::markFullResync()
{
	m_full_resync = true;
	m_pending_changes.clear();
}

std::vector<SceneChange> SceneDirtyTracker::consumeChanges()
{
	if (m_full_resync)
	{
		m_full_resync = false;
		m_pending_changes.clear();
		return { SceneChange{ {}, SceneDirtyFlagBit(SceneDirtyFlag::FullResync) } };
	}

	std::vector<SceneChange> changes;
	changes.reserve(m_pending_changes.size());
	for (const auto& pair : m_pending_changes)
		changes.push_back(pair.second);
	m_pending_changes.clear();
	return changes;
}
