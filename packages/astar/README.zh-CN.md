[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# astar

Zero-heap 2D grid A* shortest pathfinding engine with 4-way Manhattan & 8-way Octile diagonal search and path reconstruction.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:astar` ([https://github.com/evenfurther/pathfinding](https://github.com/evenfurther/pathfinding))

| 元信息 | 值 |
| --- | --- |
| 包名 | `astar` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add astar
```

```cp
import {
    GridPos,
    AStarResult,
    GridMap,
    absDiffUsize,
    heuristic,
} from "astar";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `GridPos` | `class` | 字段: `x: u16, y: u16` |
| `AStarResult` | `class` | 字段: `found: bool, cost: i64, path_len: usize` |
| `GridMap` | `class` | 字段: `width: usize, height: usize, blocked: [1024]bool` · 方法: `init, setBlocked, isBlocked, findPath` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `absDiffUsize` | `(a: usize, b: usize): i64` | 调用 `absDiffUsize` 并返回 `i64`。 |
| `heuristic` | `(x1: usize, y1: usize, x2: usize, y2: usize, allow_diagonal: bool): i64` | 调用 `heuristic` 并返回 `i64`。 |

## 最小调用示例

```cp
import { absDiffUsize } from "astar";

pub fn main() {
    let result = absDiffUsize(0, 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
