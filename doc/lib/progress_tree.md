# `cap:progress_tree` — 零分配层级进度树与节流渲染引擎

> 参考来源：`/data/dev/bun/src/bun_core/Progress.cp` | 吸收日期：2026-11-22

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

## 上游设计哲学与边界注意事项

1. **非分配、不可失败的固定槽位架构**：
   - 继承自 Bun `Progress.cp`（源自 Zig `std.Progress` 设计）：进度树内部使用固定 32 个静态节点槽位，零堆分配、不抛出 OOM 错误；当树槽位满时需及时调用 `end(node)` 归还已完成子节点。
2. **短任务防闪烁与节流刷新（`shouldRefresh`）**：
   - 通过 `initial_delay_ns` 延迟首次绘制，避免毫秒级短命令在终端一闪而过；通过 `refresh_rate_ns` 限制渲染频率，消除高频 `completeOne` 导致的终端 I/O 瓶颈。
3. **未知总数语义（`total == 0`）**：
   - 创建节点时传入 `total = 0` 表示预估总数未知（如流式下载），此时 `percent(node)` 安全返回 `0` 不发生除零，`formatLine(out)` 仅展示已完成计数或字节量。
