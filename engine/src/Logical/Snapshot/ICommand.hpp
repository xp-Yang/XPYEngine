#ifndef Snapshot_ICommand_hpp
#define Snapshot_ICommand_hpp

#include <string>

class Scene;

namespace Snapshot {

// ICommand 是 UndoRedoStack 中保存的抽象历史项。
// 它表示“已经提交的一次可撤销用户操作”，可以执行 undo/redo，并提供给菜单或调试 UI 使用的描述文本。
// 它不是编辑中的 Transaction；Transaction 只负责捕获 before/after，提交后才生成 ICommand。
class ICommand {
public:
	virtual ~ICommand() = default;

	virtual void undo(Scene& scene) = 0;
	virtual void redo(Scene& scene) = 0;
	virtual const std::string& label() const = 0;
};

} // namespace Snapshot

#endif // !Snapshot_ICommand_hpp
