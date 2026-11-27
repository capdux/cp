[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# tinyvec

Zero-heap 100%-safe inline stack vector (TinyVecI64 & TinyVecU32) with insert, swapRemove, dedup, reverse, and binarySearch (clean-room port of Rust tinyvec)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:tinyvec` ([https://github.com/Lokathor/tinyvec](https://github.com/Lokathor/tinyvec))

| 元信息 | 值 |
| --- | --- |
| 包名 | `tinyvec` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add tinyvec
```

```cp
import { TinyVecI64, TinyVecU32 } from "tinyvec";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `TinyVecI64` | `class` | 字段: `data: [64]i64, used: usize` · 方法: `new, len, capacity, isEmpty, isFull, get, +14 项` |
| `TinyVecU32` | `class` | 字段: `data: [64]u32, used: usize` · 方法: `new, len, isEmpty, push, pop, get, +2 项` |

## 最小调用示例

```cp
import { TinyVecI64 } from "tinyvec";

pub fn main() {
    // Refer to src/index.d.cp for TinyVecI64 fields and methods
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
