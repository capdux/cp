# @cap/toposort

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:topological-sort` ([https://github.com/gifnksm/topological-sort-rs](https://github.com/gifnksm/topological-sort-rs))（原库名 `topological-sort` → CP 包名 `@cap/toposort`）

零堆分配的 CP 确定性 Kahn 拓扑排序、有向图环路检测与 DAG 并行执行层级分层引擎 — 洁净室 MIT 实现，对标 Rust `topological-sort` / `petgraph::algo::toposort` 与 npm `toposort`。

Zero-allocation deterministic Kahn topological sort, cycle detection, and parallel DAG level stratification for CP — clean-room MIT parity with Rust `topological-sort`, `petgraph`, and npm `toposort`.

## 安装 / Install

```bash
cap add @cap/toposort
```

## API 参考 / API Reference

- **`TopoResult`**：`{ ok: bool, count: usize, order: [128]u16, levels: [128]u16 }`
- **`TopoGraph`**（最多 128 节点、512 有向边，压缩邻接表存储）：
  - `init(node_count)`, `reset(*_, node_count)`
  - `add_edge(*_, from, to) -> bool`
  - `in_degree(node) -> u16`, `out_degree(node) -> u16`
  - `sort() -> TopoResult`（最小节点 ID 优先队列，字典序最小确定性输出 + 并行层级 `levels`）
  - `has_cycle() -> bool`

## 使用示例 / Usage

```cp
import { TopoGraph } from "@cap/toposort";

let g = TopoGraph.init(4 as usize);
* = g.add_edge(0 as usize, 1 as usize);
* = g.add_edge(0 as usize, 2 as usize);
* = g.add_edge(1 as usize, 3 as usize);
* = g.add_edge(2 as usize, 3 as usize);

const res = g.sort();
// res.ok == true, res.order[0..4] == [0, 1, 2, 3], res.levels[0..4] == [0, 1, 1, 2]
```

## License

MIT
