[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# rangemap

Zero-allocation sorted disjoint half-open interval set [start, end) with automatic coalescing, subtraction/splitting, binary-search lookup, and span calculation (Rust rangemap / npm merge-ranges parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:rangemap` ([https://github.com/jeffparsons/rangemap](https://github.com/jeffparsons/rangemap))（原库名 `rangemap` → CP 包名 `rangemap`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `rangemap` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add rangemap
```

```cp
import { Range, RangeSet } from "rangemap";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Range` | `class` | 字段: `start: i64, end: i64` |
| `RangeSet` | `class` | 字段: `ranges: [MAX_RANGES]Range, count: usize` · 方法: `init, len, isEmpty, clear, get, insert, +6 项` |

## 最小调用示例

```cp
import { Range } from "rangemap";

pub fn main() {
    // Refer to src/index.d.cp for Range fields and methods
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
