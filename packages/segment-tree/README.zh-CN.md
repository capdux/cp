[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# segment-tree

Zero-allocation iterative 2*N Segment Tree with O(log N) rangeSum, rangeMin, rangeMax, and point updates (Rust segment-tree parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:segment-tree` ([https://github.com/Darksonn/segment-tree](https://github.com/Darksonn/segment-tree))

| 元信息 | 值 |
| --- | --- |
| 包名 | `segment-tree` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add segment-tree
```

```cp
import { RangeStats, SegmentTree, minI64, maxI64 } from "segment-tree";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `RangeStats` | `class` | 字段: `sum: i64, min_val: i64, max_val: i64, valid: bool` |
| `SegmentTree` | `class` | 字段: `size: usize, sums: [TREE_CAP]i64, mins: [TREE_CAP]i64, maxs: [TREE_CAP]i64` · 方法: `init, fromSlice, len, get, set, add, +4 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `minI64` | `(a: i64, b: i64): i64` | 调用 `minI64` 并返回 `i64`。 |
| `maxI64` | `(a: i64, b: i64): i64` | 调用 `maxI64` 并返回 `i64`。 |

## 最小调用示例

```cp
import { minI64 } from "segment-tree";

pub fn main() {
    let result = minI64(0, 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
