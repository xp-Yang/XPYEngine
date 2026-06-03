#ifndef Snapshot_Transaction_hpp
#define Snapshot_Transaction_hpp

#include "Logical/Snapshot/ICommand.hpp"
#include "Logical/Snapshot/ObjectSnapshot.hpp"

#include <memory>
#include <string>
#include <unordered_map>

class Scene;

namespace Snapshot {

// Transaction 表示一次用户意图的临时捕获器。
// 它只存在于编辑进行中：开始编辑时捕获 before，结束编辑时捕获 after，
// commit() 后生成已经提交的 ICommand 并交给 UndoRedoStack 保存。
// 同一个对象的 before 只保留第一次状态，after 保留最终状态，用于处理多帧拖拽和多对象编辑。
class Transaction {
public:
	Transaction(Scene& scene, std::string label);

	void captureBefore(GObjectID id);
	void captureAfter(GObjectID id);

	bool hasChanges() const;
	std::unique_ptr<ICommand> commit();

private:
	Scene& m_scene;
	std::string m_label;
	std::unordered_map<GObjectID, ObjectSnapshot> m_before;
	std::unordered_map<GObjectID, ObjectSnapshot> m_after;
};

} // namespace Snapshot

#endif // !Snapshot_Transaction_hpp
