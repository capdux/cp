[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# bumpalo

Zero-allocation alignment-aware bump arena allocator (BumpArena, BumpSlice) with checkpoint/rewind and overflow protection (Rust bumpalo v3 parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:bumpalo` ([https://github.com/fitzgen/bumpalo](https://github.com/fitzgen/bumpalo))

| 元信息 | 值 |
| --- | --- |
| 包名 | `bumpalo` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add bumpalo
```

```cp
import {
    BumpSlice,
    BumpArena,
    bmpIsValidAlign,
    bmpAlignUp,
    bmpHandleValid,
} from "bumpalo";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `BumpSlice` | `class` | 字段: `offset: usize, len: usize` · 方法: `empty, isEmpty, end` |
| `BumpArena` | `class` | 字段: `buf: [4096]u8, cursor: usize, alloc_count: usize` · 方法: `new, capacity, allocatedBytes, remaining, allocationCount, reset, +9 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `bmpIsValidAlign` | `(alignment: usize): bool` | 调用 `bmpIsValidAlign` 并返回 `bool`。 |
| `bmpAlignUp` | `(offset: usize, alignment: usize): usize?` | 调用 `bmpAlignUp` 并返回 `usize?`。 |
| `bmpHandleValid` | `(handle: BumpSlice, cursor: usize): bool` | 调用 `bmpHandleValid` 并返回 `bool`。 |

## 最小调用示例

```cp
import { bmpIsValidAlign } from "bumpalo";

pub fn main() {
    let result = bmpIsValidAlign(0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
