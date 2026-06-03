#include "Logical/Snapshot/ObjectSnapshotCommand.hpp"

#include "Logical/Snapshot/ObjectSnapshotService.hpp"

#include <utility>

namespace Snapshot {

ObjectSnapshotCommand::ObjectSnapshotCommand(std::string label,
	std::vector<ObjectSnapshot> before,
	std::vector<ObjectSnapshot> after)
	: m_label(std::move(label))
	, m_before(std::move(before))
	, m_after(std::move(after))
{
}

void ObjectSnapshotCommand::undo(Scene& scene)
{
	for (auto it = m_before.rbegin(); it != m_before.rend(); ++it)
		ObjectSnapshotService::restore(scene, *it);
}

void ObjectSnapshotCommand::redo(Scene& scene)
{
	for (const ObjectSnapshot& snapshot : m_after)
		ObjectSnapshotService::restore(scene, snapshot);
}

} // namespace Snapshot
