[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **crates.io** [`roaring`](https://github.com/RoaringBitmap/roaring-rs)（`MIT OR Apache-2.0`）（包名 `@cap/roaring`）。

# @cap/roaring

Zero-allocation Roaring Bitmap (16-bit high-key containers with compact sorted 16-bit arrays) for CP — clean-room MIT parity with Rust `roaring`.

纯 CP 零堆分配压缩位图（Roaring Bitmap），按高 16 位分桶、低 16 位有序紧凑数组存储，支持二分查找、空桶自动回收、`rank` 排名统计及高速集合交并差运算。

## Install / 安装

```bash
cap add @cap/roaring
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `RoaringBitmap` | `init()`, `clear(*_)` | Fixed-capacity Roaring Bitmap (up to 16 high-key containers × 64 items) |
| `insert` / `remove` / `contains` | `(value: u32) -> bool` | Binary-search insert, remove (reclaims empty containers), and lookup |
| `len` / `isEmpty` / `min` / `max` | `() -> u64 / bool / i64` | Cardinality and extrema (`-1` when empty) |
| `rank` | `(value: u32) -> u64` | Count of elements `<= value` |
| `unionWith` / `intersectWith` / `differenceWith` | `(other: RoaringBitmap) -> RoaringBitmap` | Sorted two-pointer set algebra |

## Usage / 使用示例

```cp
import { RoaringBitmap } from "@cap/roaring";

let a = RoaringBitmap.init();
* = a.insert(42 as u32);
* = a.insert(70000 as u32);
const r = a.rank(50000 as u32); // 1
```
