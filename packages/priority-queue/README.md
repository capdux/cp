[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# priority-queue

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:priority-queue` ([https://github.com/garro95/priority-queue](https://github.com/garro95/priority-queue))

Zero-allocation Indexed Binary Min-Heap with O(log N) dynamic `changePriority` and arbitrary `remove` for CP — clean-room MIT parity with Rust `priority-queue` and npm `heap-js`.

纯 CP 零堆分配索引最小堆（支持 `id` 范围 `0..127`），支持 O(log N) 按 ID 动态调权与任意节点删除，适用于 Dijkstra 最短路、A* 寻路、定时器事件调度与限流队列。

## Install / 安装

```bash
cap add priority-queue
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `PqItem` | `struct { id: u16, priority: i64, valid: bool }` | Peek/pop result entry |
| `IndexedMinHeap` | `init()`, `len()`, `isEmpty()`, `contains(id)`, `priorityOf(id)`, `peek()` | Fixed-capacity (128 items) indexed min-heap |
| `push` | `(*_, id: u16, priority: i64) -> bool` | Insert or update priority in-place |
| `pop` | `(*_) -> PqItem` | Pop lowest-priority item |
| `changePriority` | `(*_, id: u16, new_priority: i64) -> bool` | O(log N) decrease/increase key |
| `remove` | `(*_, id: u16) -> bool` | O(log N) arbitrary removal by `id` |
| `clear` | `(*_)` | Clear heap in O(size) |

## Usage / 使用示例

```cp
import { IndexedMinHeap } from "priority-queue";

let pq = IndexedMinHeap.init();
* = pq.push(1 as u16, 50 as i64);
* = pq.push(2 as u16, 20 as i64);
* = pq.changePriority(1 as u16, 10 as i64);
const top = pq.pop(); // id == 1, priority == 10
```
