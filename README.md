# 项目介绍

**XPYEngine** 是一个基于 C++17 与 OpenGL 的实时 3D 渲染引擎，适用于模型预览、材质调试、场景编辑与渲染管线实验。

## 核心特性

- **GObject + Component 场景模型** — 树状对象层级配合 Transform、Mesh、Camera、Light 等组件，逻辑与渲染解耦，扩展新行为只需挂载组件
- **基于反射的项目序列化** — `.proj` JSON 工程文件，经 TinyReflection 读写场景对象、材质、相机与光源，编辑器属性面板与序列化共用同一套元数据
- **Deferred 渲染路径** — GBuffer → 延迟光照 → 透明物体 → 后处理，在编辑器Console 可切换 Forward 路径作对照
- **PBR 材质模型** — 支持 albedo / metallic / roughness / normal 等工作流，运行时可在 PBR 与 Blinn-Phong 之间切换
- **IBL 环境光照** — 启动时从 HDR 或 Skybox Cubemap 预计算 irradiance、prefilter 与 BRDF LUT，供延迟/前向光照采样
- **SSAO** — PBR 模式下基于 GBuffer 的屏幕空间环境光遮蔽，参数可在 Console 调节
- **Bloom、FXAA、Tone Mapping 等后处理** — 延迟路径下按开关组合
- **GPU Instancing** — 点光源可视化等场景通过 instancing buffer 批量绘制，减少 draw call
- **骨骼动画** — Assimp 导入动画片段，`AnimationSystem` 计算骨骼矩阵并驱动蒙皮网格
- **RenderGraph + RenderPath + RenderPass** — RenderPath描述整条管线，RenderGraph 每帧声明 RenderPass 依赖与中间资源，RenderPass 通过 slot 绑定解耦具体纹理名
- **RenderGraph 调试视图** — Debug Window 浏览 GBuffer、SceneColor 等中间纹理，以及 Pass 执行顺序 dump
- **RHI 抽象层** — 统一 Buffer / Texture / GraphicsPipeline / CommandBuffer 等接口，当前仅 OpenGL 后端，便于后续接入其他图形 API
- **反射驱动的 GUI 编辑面板** — Scene Hierarchy 基于 TinyReflection 自动生成 Transform、材质、光源、相机等字段编辑器
- **信号槽机制** — GObject 脏标记、GPU 拾选结果等通过 Signal/Slot 广播，驱动 Render 增量同步与选中状态更新
- **跨平台设计** — Windows / macOS 完整支持；Linux 可编译运行

## 功能展示

### 编辑器布局

Docking 主界面：MainCanvas 预览、Scene Hierarchy 与 Console 参数面板。

![编辑器布局](docs/images/editor_layout.png)

### 延迟渲染与 PBR

Deferred 路径下的实时光照、IBL 与 SSAO，可在 Console 中切换材质模型与效果开关。

![延迟渲染](docs/images/deferred_pbr.png)

### RenderGraph 调试

Debug Window 中查看 GBuffer、SceneColor 等中间资源与 Pass 执行顺序。

![RenderGraph 调试](docs/images/render_graph_debug.png)

### 场景编辑

层级树选中对象后，ImGuizmo 变换与反射驱动的属性面板。

![场景编辑](docs/images/scene_editing.png)

# 架构设计

| 文档 | 内容 |
|------|------|
| [架构.md](docs/架构.md) | 分层架构（Base / Platform / AssetManager / Logical / Render / GUI）、Scene 子系统、RenderGraph 管线、GUI 与主循环 |
| [资源管理.md](docs/资源管理.md) | 磁盘 / CPU / GPU 三层资源生命周期、ProjectDTO 序列化、Logical → RenderScene 脏标记同步 |

# 快速开始

## 依赖

### 平台要求

| 项目 | 要求 |
|------|------|
| 构建工具 | [CMake](https://cmake.org/) ≥ 3.13 |
| 语言标准 | C++17 |
| 图形 API | OpenGL 4.3 Core（Windows / Linux）；OpenGL 4.1 Core + Forward Compatible（macOS） |
| 操作系统 | Windows 10+、macOS、Linux（Linux 下文件对话框尚未实现） |

### 第三方库

依赖位于 `thirdparty/`，由 CMake 随引擎一并编译，无需单独安装：

| 类别 | 库 |
|------|-----|
| 图形与窗口 | glad、glfw |
| GUI | imgui、ImGuizmo |
| 资源 | stb、assimp |
| 数学与工具 | glm、spdlog、json11 |
| 反射 / 序列化 | TinyReflection |

## 构建

在项目根目录执行 **out-of-source** 构建（不支持在源码目录内直接生成）：

```bash
cmake -S . -B build
cmake --build build --config Release
```

单配置生成器（如 Ninja）：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

主要构建产物：

| 目标 | 说明 |
|------|------|
| **EngineRuntime** | 引擎静态库，源码位于 `engine/src/`，公开头文件位于 `engine/include/` |
| 着色器与内置资产 | `engine/asset/`（构建时通过 `ASSET_DIR` 宏注入路径） |

MSVC 下根 CMake 自动添加 `/utf-8` 编译选项。

## 集成方式

链接 `EngineRuntime` 并包含公开头文件即可使用引擎 API：

```cpp
#include "Engine.hpp"

int main()
{
    auto& engine = Engine::get();
    engine.init();

    // 在此向 Scene 添加对象、加载模型或项目
    // engine.Scene()->loadModel("path/to/model.obj");
    // engine.Scene()->loadProject("path/to/scene.proj");

    engine.run();
    engine.shutdown();
    return 0;
}
```
