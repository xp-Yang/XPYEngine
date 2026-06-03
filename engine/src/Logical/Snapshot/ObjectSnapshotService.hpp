#ifndef ObjectSnapshotService_hpp
#define ObjectSnapshotService_hpp

#include "Logical/Snapshot/ObjectSnapshot.hpp"
#include "Logical/Framework/World/SceneDirty.hpp"

class GObject;
class Scene;
class SceneObjectRegistry;

namespace Snapshot {

// ObjectSnapshotService 负责快照的捕获、恢复和比较。
// GUI、Transaction 和具体 ICommand 不直接操作 DTO 细节，统一通过这里生成和恢复 GObject 的编辑态。
// 第一版快照路径使用当前对象中的路径，不做工程保存时的相对路径转换。
class ObjectSnapshotService {
public:
	static ObjectSnapshot capture(Scene& scene, GObjectID id);
	static void restore(Scene& scene, const ObjectSnapshot& snapshot);
	static bool equals(const ObjectSnapshot& lhs, const ObjectSnapshot& rhs);

	static ObjectDTO buildDTOFromObject(GObject& object);
	static void applyDTOToObject(Scene& scene, GObject& object, const ObjectDTO& dto);
	static void applyDTOToObject(SceneObjectRegistry& registry, GObject& object, const ObjectDTO& dto);
	static SceneDirtyFlags dirtyFlagsForDTO(const ObjectDTO& dto);

private:
	static GObject* createObjectFromDTO(Scene& scene, GObjectID id, const ObjectDTO& dto);
};

} // namespace Snapshot

#endif // !ObjectSnapshotService_hpp
