#ifndef ObjectSnapshot_hpp
#define ObjectSnapshot_hpp

#include "AssetManager/DTO.hpp"
#include "Logical/Framework/World/IDAllocator.hpp"

namespace Snapshot {

// ObjectSnapshot 表示一个 GObject 在某个时间点的“可编辑状态”。
// 它不是运行时对象的深拷贝，不保存 GPU/RHI 资源、渲染缓存或完整 mesh 顶点数据。
// dto 只承载可以从场景编辑器恢复的状态，例如名称、可见性、Transform、材质覆盖、相机和光源参数。
// existed 用来表达对象在该时间点是否存在，因此同一结构可以覆盖创建、删除和普通编辑。
struct ObjectSnapshot {
	GObjectID id{};
	ObjectDTO dto{};
	bool existed{ false };
};

} // namespace Snapshot

#endif // !ObjectSnapshot_hpp
