#include "Logical/Snapshot/UndoRedoStack.hpp"

#include <utility>

namespace Snapshot {

void UndoRedoStack::execute(std::unique_ptr<ICommand> command, Scene& scene)
{
	if (!command)
		return;
	command->redo(scene);
	pushExecuted(std::move(command));
}

void UndoRedoStack::pushExecuted(std::unique_ptr<ICommand> command)
{
	if (!command)
		return;

	const int current_index = static_cast<int>(m_undo_stack.size());
	if (m_clean_index > current_index)
		m_clean_index = -1;

	m_redo_stack.clear();
	m_undo_stack.push_back(std::move(command));
	trimUndoStackIfNeeded();
}

void UndoRedoStack::undo(Scene& scene)
{
	if (m_undo_stack.empty())
		return;

	std::unique_ptr<ICommand> command = std::move(m_undo_stack.back());
	m_undo_stack.pop_back();
	command->undo(scene);
	m_redo_stack.push_back(std::move(command));
}

void UndoRedoStack::redo(Scene& scene)
{
	if (m_redo_stack.empty())
		return;

	std::unique_ptr<ICommand> command = std::move(m_redo_stack.back());
	m_redo_stack.pop_back();
	command->redo(scene);
	m_undo_stack.push_back(std::move(command));
	trimUndoStackIfNeeded();
}

void UndoRedoStack::clear()
{
	m_undo_stack.clear();
	m_redo_stack.clear();
	m_clean_index = 0;
}

void UndoRedoStack::setClean()
{
	m_clean_index = static_cast<int>(m_undo_stack.size());
}

bool UndoRedoStack::isClean() const
{
	return m_clean_index >= 0 && m_clean_index == static_cast<int>(m_undo_stack.size());
}

void UndoRedoStack::trimUndoStackIfNeeded()
{
	if (m_max_history <= 0)
		return;

	while (static_cast<int>(m_undo_stack.size()) > m_max_history) {
		m_undo_stack.erase(m_undo_stack.begin());
		if (m_clean_index > 0)
			--m_clean_index;
		else
			m_clean_index = -1;
	}
}

} // namespace Snapshot
