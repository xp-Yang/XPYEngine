#ifndef RenderGraphDumper_hpp
#define RenderGraphDumper_hpp

#include "Render/Graph/RenderGraphDebugInfo.hpp"
#include "Render/Graph/RenderGraph.hpp"

#include <string>
#include <vector>

// RenderGraph 调试导出器：只读访问图状态，生成资源列表和执行顺序文本。
class RenderGraphDumper {
public:
    explicit RenderGraphDumper(const RenderGraph& graph);

    std::vector<std::string> resourceNames() const;
    std::vector<RenderGraphResourceDebugInfo> resourceInfos() const;
    std::string graph() const;
    std::string executionOrder() const;

private:
    const RenderGraph& m_graph;
};

#endif // !RenderGraphDumper_hpp
