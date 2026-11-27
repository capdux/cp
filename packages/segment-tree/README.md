[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# segment-tree

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:segment-tree` ([https://github.com/Darksonn/segment-tree](https://github.com/Darksonn/segment-tree))

Zero-allocation iterative 2×N Segment Tree simultaneously supporting O(log N) `rangeSum`, `rangeMin` (RMQ), `rangeMax`, and point updates (`set` / `add`) for CP — clean-room MIT parity with Rust `segment-tree`.

纯 CP 零堆分配迭代式线段树（最大容量 256），O(N) 自底向上建树，O(log N) 单次查询同步返回区间和/最小值/最大值，适用于时序指标聚合、金融 K 线极值查询与在线流式分析。

## Install / 安装

```bash
cap add segment-tree
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `RangeStats` | `struct { sum: i64, min_val: i64, max_val: i64, valid: bool }` | Combined range query result |
| `SegmentTree` | `init(n)`, `fromSlice(values)`, `len()`, `get(idx)` | Fixed-capacity (256) iterative segment tree |
| `set` / `add` | `(*_, idx: usize, val/delta: i64) -> bool` | O(log N) point update |
| `query` | `(start: usize, end_exclusive: usize) -> RangeStats` | O(log N) combined sum/min/max query |
| `rangeSum` / `rangeMin` / `rangeMax` | `(start: usize, end_exclusive: usize) -> i64` | Convenience range accessors |

## Usage / 使用示例

```cp
import { SegmentTree } from "segment-tree";

const prices: [5]i64 = [105, 98, 112, 95, 120];
let st = SegmentTree.fromSlice(prices[0..5]);
const stats = st.query(1 as usize, 4 as usize);
// stats.sum == 305, stats.min_val == 95, stats.max_val == 112
```
