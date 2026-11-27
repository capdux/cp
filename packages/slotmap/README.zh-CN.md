[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# slotmap

Zero-allocation generational-index slot map (SlotKey, SlotMapI64, SecondaryMapI64) with O(1) free-list and strict ABA protection (Rust slotmap v1 parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:slotmap` ([https://github.com/orlp/slotmap](https://github.com/orlp/slotmap))

| 元信息 | 值 |
| --- | --- |
| 包名 | `slotmap` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add slotmap
```

```cp
import {
    SlotKey,
    SlotMapI64,
    SecondaryMapI64,
    slmIsOccupiedVersion,
    slmAllocVersion,
    slmFreeVersion,
} from "slotmap";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `SlotKey` | `class` | 字段: `idx: u32, version: u32` · 方法: `nullKey, isNull, eq, toU64, fromU64` |
| `SlotMapI64` | `class` | 字段: `values: [128]i64, versions: [128]u32, next_free: [128]u32, free_head: u32, count: usize` · 方法: `new, len, capacity, isEmpty, containsKey, insert, +4 项` |
| `SecondaryMapI64` | `class` | 字段: `values: [128]i64, versions: [128]u32, count: usize` · 方法: `new, len, containsKey, insert, get, remove, +1 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `slmIsOccupiedVersion` | `(version: u32): bool` | 调用 `slmIsOccupiedVersion` 并返回 `bool`。 |
| `slmAllocVersion` | `(cur_version: u32): u32` | 调用 `slmAllocVersion` 并返回 `u32`。 |
| `slmFreeVersion` | `(cur_version: u32): u32` | 调用 `slmFreeVersion` 并返回 `u32`。 |

## 最小调用示例

```cp
import { slmIsOccupiedVersion } from "slotmap";

pub fn main() {
    let result = slmIsOccupiedVersion(0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
