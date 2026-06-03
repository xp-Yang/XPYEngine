#ifndef Command_hpp
#define Command_hpp

class Scene;

class ICommand {
public:
	virtual ~ICommand() = default;

	virtual void undo(Scene& scene) = 0;
	virtual void redo(Scene& scene) = 0;
	virtual const char* description() const = 0;
	virtual bool empty() const { return false; }
};

#endif // !Command_hpp
