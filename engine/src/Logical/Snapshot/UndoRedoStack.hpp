#ifndef UndoRedoStack_hpp
#define UndoRedoStack_hpp

#include "Logical/Snapshot/ICommand.hpp"

#include <memory>
#include <vector>

class Scene;

namespace Snapshot {

// UndoRedoStack 是场景编辑历史栈。
// 它只保存已经提交的 ICommand：undo 栈表示当前状态之前的用户操作，
// redo 栈表示被撤销后可以重新执行的用户操作。
// pushExecuted() 用于对象已经被外部编辑完成后的入栈，execute() 用于命令自身负责 redo 的场景。
class UndoRedoStack {
public:
	void execute(std::unique_ptr<ICommand> command, Scene& scene);
	void pushExecuted(std::unique_ptr<ICommand> command);

	void undo(Scene& scene);
	void redo(Scene& scene);

	bool canUndo() const { return !m_undo_stack.empty(); }
	bool canRedo() const { return !m_redo_stack.empty(); }

	void clear();
	void setClean();
	bool isClean() const;

	void setMaxHistory(int max_history) { m_max_history = max_history; }
	int maxHistory() const { return m_max_history; }

private:
	void trimUndoStackIfNeeded();

	std::vector<std::unique_ptr<ICommand>> m_undo_stack;
	std::vector<std::unique_ptr<ICommand>> m_redo_stack;
	int m_clean_index{ 0 };
	int m_max_history{ 100 };
};

} // namespace Snapshot

#endif // !UndoRedoStack_hpp
