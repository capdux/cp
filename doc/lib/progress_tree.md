# `cap:progress_tree`

纯 CP 零堆分配层级进度树与节流渲染引擎，吸收自 `/data/dev/bun/src/bun_core/Progress.cp`。

## 导出类型与方法

- `ProgressUnit`：`none | files | bytes`
- `ProgressNode`：单节点进度状态（名称、单位、已完成/预估总数、父子链路）
- `ProgressTree`：固定 32 节点层级进度树
  - `init(start_ns)` / `startRoot(name, total)` / `startChild(parent, name, total, unit)`
  - `completeOne(node)` / `addCompleted(node, delta)` / `setName(node, name)` / `end(node)`
  - `shouldRefresh(now_ns)`：遵循 `initial_delay_ns` 与 `refresh_rate_ns` 的确定性节流判定
  - `percent(node)`：返回 `0..100` 进度百分比
  - `formatLine(out)`：渲染活跃链路（如 `[1/3] Installing > Downloading (512KB/1MB)`）
