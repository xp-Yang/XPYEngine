#include "Logical/Snapshot/Transaction.hpp"

#include "Logical/Snapshot/ObjectSnapshotCommand.hpp"
#include "Logical/Snapshot/ObjectSnapshotService.hpp"

#include <algorithm>
#include <utility>
#include <vector>

namespace Snapshot {

Transaction::Transaction(Scene& scene, std::string label)
	: m_scene(scene)
	, m_label(std::move(label))
{
}

void Transaction::captureBefore(GObjectID id)
{
	if (!id.isValid())
		return;
	if (m_before.find(id) != m_before.end())
		return;
	m_before.emplace(id, ObjectSnapshotService::capture(m_scene, id));
}

void Transaction::captureAfter(GObjectID id)
{
	if (!id.isValid())
		return;
	m_after[id] = ObjectSnapshotService::capture(m_scene, id);
}

bool Transaction::hasChanges() const
{
	for (const auto& pair : m_before) {
		auto it = m_after.find(pair.first);
		if (it != m_after.end() && !ObjectSnapshotService::equals(pair.second, it->second))
			return true;
	}
	return false;
}

std::unique_ptr<ICommand> Transaction::commit()
{
	std::vector<GObjectID> ids;
	ids.reserve(m_before.size());
	for (const auto& pair : m_before)
		ids.push_back(pair.first);
	std::sort(ids.begin(), ids.end());

	std::vector<ObjectSnapshot> before;
	std::vector<ObjectSnapshot> after;
	before.reserve(ids.size());
	after.reserve(ids.size());

	for (GObjectID id : ids) {
		const ObjectSnapshot& before_snapshot = m_before.at(id);
		ObjectSnapshot after_snapshot;
		auto after_it = m_after.find(id);
		if (after_it != m_after.end())
			after_snapshot = after_it->second;
		else
			after_snapshot = ObjectSnapshotService::capture(m_scene, id);

		if (ObjectSnapshotService::equals(before_snapshot, after_snapshot))
			continue;

		before.push_back(before_snapshot);
		after.push_back(std::move(after_snapshot));
	}

	if (before.empty())
		return nullptr;

	return std::make_unique<ObjectSnapshotCommand>(m_label, std::move(before), std::move(after));
}

} // namespace Snapshot
