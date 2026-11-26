# @cap/fenwick

Zero-allocation Binary Indexed Tree (Fenwick Tree) with O(log N) prefix/range sums, O(N) linear construction, and O(log N) binary-lifting `lowerBound` for CP — clean-room MIT parity with Rust `fenwick`.

纯 CP 零堆分配树状数组（最大容量 256），支持 O(log N) 前缀和/区间和查询、单点增量/赋值更新与 O(log N) 二分提升排名统计，适用于实时指标统计、频率直方图、限流与顺序统计。

## Install / 安装

```bash
cap add @cap/fenwick
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `FenwickTree.init` | `(len: usize) -> FenwickTree` | Create zeroed tree (clamped to 256) |
| `FenwickTree.fromSlice` | `(values: []const i64) -> FenwickTree` | O(N) linear-time construction |
| `len` / `total` / `reset` | `()` | Size, full sum, and zero reset |
| `add` / `set` / `get` | `(idx: usize, ...)` | 0-based point update and query |
| `prefixSum` / `rangeSum` | `(start, end_exclusive) -> i64` | Half-open `[0, end)` and `[start, end)` sums |
| `lowerBound` | `(target: i64) -> usize` | O(log N) binary-lifting smallest `idx` with `prefixSum(idx + 1) >= target` |

## Usage / 使用示例

```cp
import { FenwickTree } from "@cap/fenwick";

const weights: [4]i64 = [2, 3, 5, 10];
let ft = FenwickTree.fromSlice(weights[0..4]);
const sum = ft.rangeSum(1 as usize, 3 as usize); // 8
const k = ft.lowerBound(6 as i64);               // 2 (prefixSum(3) == 10 >= 6)
```
