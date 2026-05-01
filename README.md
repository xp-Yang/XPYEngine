# 依赖与构建流程

### 1. 主要依赖

依赖定义见 `engine/thirdparty/CMakeLists.txt` 与 `engine/src/CMakeLists.txt`。

核心依赖包括：

- 渲染与窗口：`glad`, `glfw`
- GUI：`imgui`
- 资源加载：`stb`, `assimp`
- 数学与工具：`glm`, `spdlog`, `json11`
- 反射/序列化：`TinyReflection`

### 2. 构建目标

- `EngineRuntime`（库，位于 `engine/src/CMakeLists.txt`）
- `SAMPLES`（可执行程序，位于 `samples/CMakeLists.txt`，链接 `EngineRuntime`）

### 3. 构建步骤（推荐 out-of-source）

在项目根目录执行：

```bash
cmake -S . -B build
cmake --build build --config Release
```

如果使用单配置生成器（如 Ninja），可用：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### 4. 运行样例

- 入口在 `samples/main.cpp`
- 默认创建 `cubetest` demo 并进入 `engine.run()`

---

# 项目说明

## 1. 分层设计总览：Base / ResourceManager / Logical / Render / GUI

项目源码主目录在 `engine/src`，当前采用分层架构。每层职责如下：

### Base（基础设施层）
- 位置：`engine/src/Base/include`
- 典型内容：数学库封装、日志、计时器、信号槽、元数据与序列化工具、路径工具等
- 作用：为上层提供无业务语义、可复用的基础能力
- 设计原因：避免在业务代码中反复实现通用工具，减少耦合，提高可测试性

### ResourceManager（资源与数据交换层）
- 位置：`engine/src/ResourceManager`
- 典型内容：模型导入（Assimp）、GCode 导入、`ProjectDTO` 定义
- 作用：负责“外部资源/文件格式 -> 引擎内部数据”的转换
- 设计原因：把文件格式处理与运行时逻辑分离，方便替换导入器或扩展格式

### Logical（逻辑层）
- 位置：`engine/src/Logical`
- 典型内容：Scene/GObject/Component、Mesh/Material/Texture、输入控制、动画系统
- 作用：描述“场景里有什么、对象怎么组织、状态如何更新”
- 设计原因：将场景语义独立于渲染细节，保证逻辑层可以驱动不同渲染路径

### Render（渲染层）
- 位置：`engine/src/Render`
- 典型内容：`RenderSystem`、`RenderPath`、`RenderPass`、`RenderSourceData`、RHI 抽象
- 作用：把逻辑层数据转换为 GPU 可执行的绘制过程
- 设计原因：通过 Path/Pass 组合实现可扩展渲染流程（Forward/Deferred 等），降低复杂度

### GUI（编辑器与交互层）
- 位置：`engine/src/GUI`
- 典型内容：ImGui 编辑器窗体、输入转发、场景树、控制台、主预览画布
- 作用：提供编辑器界面、菜单交互、鼠标拾取和相机控制
- 设计原因：把编辑器行为独立出来，不污染逻辑与渲染核心代码

---

## 2. Logical 层核心：Framework 与核心对象

### 2.1 Framework：Scene / GObject / Component

当前默认配置 `ENABLE_ECS=false`，所以主运行模式是对象树 + 组件系统。

#### Scene
- 文件：`engine/src/Logical/include/Logical/Framework/World/Scene.hpp`、`engine/src/Logical/Framework/World/Scene.cpp`
- 职责：
  - 管理对象列表 `m_objects`
  - 管理光照与主相机
  - 处理对象拾取状态
  - 负责 `loadModel`、`loadProject`、`saveProject`

#### GObject
- 文件：`engine/src/Logical/include/Logical/Framework/Object/GObject.hpp`
- 职责：
  - 表示场景中的逻辑对象
  - 维护父子层级（树结构）
  - 持有组件列表 `std::vector<std::shared_ptr<Component>>`
  - 提供 `addComponent<T>() / getComponent<T>()`

#### Component
- 文件：`engine/src/Logical/include/Logical/Framework/Component/Component.hpp`
- 职责：
  - 作为行为/数据模块基类
  - 挂接到 `GObject`，通过 `parent_object` 回指宿主

常用组件包括：
- `TransformComponent`：TRS 变换（平移、旋转、缩放）
- `MeshComponent`：网格与材质集合（模型来源路径 + 子网格）
- `CameraComponent`：相机参数
- `AnimationComponent`：动画片段与播放参数（当前骨骼动画接入点）

### 2.2 核心资源对象：Mesh / Material / Texture

#### Mesh
- 文件：`engine/src/Logical/include/Logical/Mesh.hpp`
- 包含：顶点数组、索引数组、局部变换、材质引用
- 顶点结构含位置/法线/UV；骨骼动画版本还包含骨骼索引与权重

#### Material
- 文件：`engine/src/Logical/include/Logical/Material.hpp`
- 包含：
  - PBR 贴图：albedo/metallic/roughness/ao
  - Blinn-Phong 贴图：diffuse/specular/normal/height
  - alpha 透明度

#### Texture
- 文件：`engine/src/Logical/include/Logical/Texture.hpp`
- 描述贴图文件路径与像素数据元信息
- 支持普通 2D 纹理与 CubeTexture（天空盒）

---

## 3. Render 层设计：RenderPath 与 RenderPass

### 3.1 核心思想
- `RenderSystem` 每帧从 `Scene` 汇总数据到 `RenderSourceData`
- `RenderPath` 决定“整条渲染流程”怎么组织（如 Forward / Deferred）
- `RenderPass` 是最小渲染步骤，通常对应一个 framebuffer

### 3.2 RenderPath
- 接口：`engine/src/Render/Path/RenderPath.hpp`
- 已有路径：
  - `ForwardRenderPath`：`engine/src/Render/Path/ForwardRenderPath.cpp`
  - `DeferredRenderPath`：`engine/src/Render/Path/DeferredRenderPath.cpp`

Forward 路径典型顺序：
- Picking -> Shadow -> ForwardLighting -> Skybox -> Outline -> Combine

Deferred 路径典型顺序：
- Picking -> Shadow -> GBuffer -> DeferredLighting -> Transparent -> Bloom/Outline/Combine（按开关）

### 3.3 RenderPass
- 基类：`engine/src/Render/Pass/RenderPass.hpp`
- 每个 Pass：
  - 持有 framebuffer
  - 通过 `prepareRenderSourceData` 获取当前帧渲染输入
  - 在 `draw()` 内执行具体绘制

这种设计让“流程编排”（Path）和“单步实现”（Pass）解耦，便于扩展新效果。

---

## 4. Project 功能：序列化与反序列化

项目相关逻辑分为两层：

### 4.1 ProjectManager（项目元信息）
- 文件：`engine/src/Project/ProjectManager.hpp`、`engine/src/Project/ProjectManager.cpp`
- 职责：
  - 打开/保存项目文件基本信息（schema_version, project_name 等）
  - 维护当前项目路径

### 4.2 Scene 的场景数据序列化（核心）
- 数据结构：`engine/src/ResourceManager/DTO.hpp`
  - `ProjectDTO -> ObjectDTO -> SubMeshDTO / MaterialDTO / TransformDTO`
- 实现位置：`engine/src/Logical/Framework/World/Scene.cpp`
  - `buildProjectDTOFromScene`：将运行时 Scene 导出为 DTO 并写盘
  - `applyProjectDTOToScene`：从 DTO 重建场景对象/材质/变换
  - `Scene::saveProject` / `Scene::loadProject`：对外 API

也就是说：编辑器中的“保存/打开项目”最终由 Scene 的 DTO 流程完成场景级序列化。

---

## 5. GUI 层与引擎主循环

### 5.1 GUI 层粗略结构
- 核心文件：`engine/src/GUI/Editor/ImGuiEditor.cpp`
- 功能模块：
  - Docking 主布局
  - MainCanvas / PreviewCanvas
  - SceneHierarchy / Console / ContextMenu / DebugWindow
  - 菜单栏 File（Open/Open Project/Save Project）

### 5.2 输入与编辑器交互
- 文件：`engine/src/GUI/Editor/ImGuiInput.cpp`
- 作用：
  - 采集 ImGui 鼠标键盘状态
  - 驱动相机控制（拖拽、滚轮、WASD）
  - 触发 Picking（点击选中对象）

### 5.3 引擎循环（运行时主流程）
- 文件：`engine/src/Engine.cpp`
- 每帧顺序大致为：
  1. `gui_editor->beginFrame()`
  2. `gui_input->onUpdate()`
  3. `animation_system->onUpdate(scene)`（动画）
  4. `render_system->onUpdate(scene)`（渲染）
  5. `gui_editor->onUpdate()`（编辑器窗口）
  6. `gui_editor->endFrame()` + `window->swapBuffer()`

可以理解为：**输入更新逻辑，逻辑驱动渲染，GUI 最后叠加输出**。

---

# 快速导航（常看文件）
- 引擎循环：`engine/src/Engine.cpp`
- 全局上下文：`engine/src/GlobalContext.hpp`
- 场景与项目序列化：`engine/src/Logical/Framework/World/Scene.cpp`
- 渲染系统：`engine/src/Render/RenderSystem.cpp`
- 渲染流程：`engine/src/Render/Path/*.cpp`
- 编辑器：`engine/src/GUI/Editor/ImGuiEditor.cpp`

