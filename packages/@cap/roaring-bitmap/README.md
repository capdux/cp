# @cap/roaring-bitmap

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:roaring` ([https://github.com/RoaringBitmap/roaring-rs](https://github.com/RoaringBitmap/roaring-rs))（原库名 `roaring` → CP 包名 `@cap/roaring-bitmap`）

Zero-allocation compressed roaring bitmap (`RoaringBitmap`) covering the full `u32` domain (`0..=u32::MAX`) with high-16-bit sorted chunks, low-16-bit sorted containers, O(1) `min`/`max`, `rank`, `insertRange`, and full set algebra for CP — clean-room MIT parity with Rust `roaring` v0.10 (450M+ downloads).

纯 CP 零堆分配压缩位图集合，适用于搜索引擎倒排索引、用户标签人群圈选、稀疏 `u32` ID 集合运算与分析型数据库位图过滤。

## Install / 安装

```bash
cap add @cap/roaring-bitmap
```

## API

### Constants

- `RBM_MAX_CHUNKS: usize = 16` — Maximum distinct high-16-bit chunks
- `RBM_CHUNK_CAP: usize = 64` — Maximum elements per chunk

### `RoaringBitmap`

| Method | Signature | Description |
| --- | --- | --- |
| `new` | `static fn new() -> RoaringBitmap` | Creates an empty roaring bitmap |
| `insert` / `remove` / `contains` | `(val: u32) -> bool` | O(log k) chunk + low-16 lookup & mutation across full `u32` |
| `len` / `isEmpty` | `() -> u64 / bool` | O(1) cardinality & emptiness check |
| `min` / `max` | `() -> u32?` | O(1) minimum and maximum element |
| `rank` | `(val: u32) -> u64` | Counts elements `<= val` |
| `insertRange` | `(*_, start: u32, end: u32) -> u64` | Inserts half-open range `[start, end)`, returns count inserted |
| `clear` | `(*_)` | Empties all chunks and resets length |
| `unionWith` / `intersectWith` / `differenceWith` | `(*_, other: RoaringBitmap)` | In-place set algebra |
| `isSubset` / `isDisjoint` | `(other: RoaringBitmap) -> bool` | Set relation predicates |

## Usage / 使用示例

```cp
import { RoaringBitmap } from "@cap/roaring-bitmap";

let bm = RoaringBitmap.new();
* = bm.insert(0x00010005 as u32);
* = bm.insert(0xFFFF0001 as u32);
const r = bm.rank(0x00010005 as u32); // 1
const mx = bm.max();                  // 0xFFFF0001
```
