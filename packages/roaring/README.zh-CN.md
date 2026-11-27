[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# roaring

Zero-allocation Roaring Bitmap with 16-bit high-key containers, binary search, rank, and set algebra (Rust roaring parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **crates.io** [`roaring`](https://github.com/RoaringBitmap/roaring-rs)（`MIT OR Apache-2.0`）（包名 `roaring`）。

| 元信息 | 值 |
| --- | --- |
| 包名 | `roaring` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add roaring
```

```cp
import {
    BinSearch,
    RoaringContainer,
    RoaringBitmap,
    highKey,
    lowVal,
    combineKeyVal,
    emptyContainer,
    searchU16,
} from "roaring";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `BinSearch` | `class` | 字段: `found: bool, pos: usize` |
| `RoaringContainer` | `class` | 字段: `key: u16, values: [MAX_CONTAINER_ITEMS]u16, count: usize` |
| `RoaringBitmap` | `class` | 字段: `containers: [MAX_CONTAINERS]RoaringContainer, container_count: usize, total_len: u64` · 方法: `init, insert, remove, contains, len, isEmpty, +7 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `highKey` | `(v: u32): u16` | 调用 `highKey` 并返回 `u16`。 |
| `lowVal` | `(v: u32): u16` | 调用 `lowVal` 并返回 `u16`。 |
| `combineKeyVal` | `(hi: u16, lo: u16): u32` | 调用 `combineKeyVal` 并返回 `u32`。 |
| `emptyContainer` | `(): RoaringContainer` | 调用 `emptyContainer` 并返回 `RoaringContainer`。 |
| `searchU16` | `(arr: [MAX_CONTAINER_ITEMS]u16, count: usize, target: u16): BinSearch` | 调用 `searchU16` 并返回 `BinSearch`。 |
| `searchContainer` | `(containers: [MAX_CONTAINERS]RoaringContainer, count: usize, key: u16): BinSearch` | 调用 `searchContainer` 并返回 `BinSearch`。 |
| `unionContainers` | `(a: RoaringContainer, b: RoaringContainer): RoaringContainer` | 调用 `unionContainers` 并返回 `RoaringContainer`。 |
| `intersectContainers` | `(a: RoaringContainer, b: RoaringContainer): RoaringContainer` | 调用 `intersectContainers` 并返回 `RoaringContainer`。 |
| `diffContainers` | `(a: RoaringContainer, b: RoaringContainer): RoaringContainer` | 调用 `diffContainers` 并返回 `RoaringContainer`。 |

## 最小调用示例

```cp
import { highKey } from "roaring";

pub fn main() {
    let result = highKey(0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
