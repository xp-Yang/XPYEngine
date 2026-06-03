#ifndef ObjectSnapshotCommand_hpp
#define ObjectSnapshotCommand_hpp

#include "Logical/Snapshot/ICommand.hpp"
#include "Logical/Snapshot/ObjectSnapshot.hpp"

#include <string>
#include <vector>

namespace Snapshot {

// ObjectSnapshotCommand 是基于 GObject 快照恢复的 ICommand 实现。
// 一条命令对应一次已经完成的用户意图，但一次用户意图可能影响多个 GObject，
// 因此 before/after 都保存为列表，用于多选 Gizmo、批量删除或未来组合操作。
class ObjectSnapshotCommand : public ICommand {
public:
	ObjectSnapshotCommand(std::string label,
		std::vector<ObjectSnapshot> before,
		std::vector<ObjectSnapshot> after);

	void undo(Scene& scene) override;
	void redo(Scene& scene) override;
	const std::string& label() const override { return m_label; }

private:
	std::string m_label;
	std::vector<ObjectSnapshot> m_before;
	std::vector<ObjectSnapshot> m_after;
};

} // namespace Snapshot

#endif // !ObjectSnapshotCommand_hpp
