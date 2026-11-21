# `cap:bitset_simd` — 按字向量化加速动态位集合

## 1. 概述

`cap:bitset_simd`（支持 `import { BitSet, BitSetError } from "cap:bitset_simd";` 与 `cap.bitset_simd` 动态挂载）是纯 CP 实现的高性能动态位集合标准库。底层基于 `u64` 字块存储，利用硬件原生 `@popCount` 与 `@ctz` 指令实现按字向量化加速的基数统计、稀疏位高速迭代与批量集合代数运算。

## 2. 核心类型

- `BitSetError`: 位集合操作错误枚举
  - `OutOfBounds`: 索引或区间超出 `bit_len` 范围
  - `SizeMismatch`: 二元集合运算两侧 `bitLen()` 不一致
  - `InvalidRange`: 区间参数满足 `start > end`
- `BitSet`: 基于 `u64` 字块的动态位集合结构体（自动维护尾部未使用高位掩码）。

## 3. API 速查

| 方法 | 说明 |
| :--- | :--- |
| `BitSet.initEmpty(alloc, bit_len) -> ?BitSet` | 初始化全 0 位集合 |
| `BitSet.initFull(alloc, bit_len) -> ?BitSet` | 初始化全 1 位集合（尾字高位自动清零） |
| `deinit(&self)` | 释放底层 `u64` 字块内存 |
| `set(&self, idx) -> ?void` | 将指定位置 1 |
| `unset(&self, idx) -> ?void` | 将指定位清 0 |
| `toggle(&self, idx) -> ?void` | 翻转指定位 |
| `isSet(self, idx) -> bool` | 查询指定位是否为 1（越界返回 `false`） |
| `setRange(&self, start, end) -> ?void` | 批量将 `[start, end)` 区间置 1 |
| `count(self) -> usize` | 基于 `@popCount` 统计置位总数 |
| `nextSetBit(self, from_idx: usize) -> usize?` | 基于 `@ctz` 查找从 `from_idx` 起的下一个置位索引 |
| `unionWith(&self, other) -> ?void` | 原地并集（`self \|= other`） |
| `intersectWith(&self, other) -> ?void` | 原地交集（`self &= other`） |
| `differenceWith(&self, other) -> ?void` | 原地差集（`self &= ~other`） |
| `xorWith(&self, other) -> ?void` | 原地对称差（`self ^= other`） |
| `equals(self, other) -> bool` | 判断两个位集合长度与各位是否完全相等 |
| `bitLen(self) -> usize` | 返回位集合总位数 |

## 4. 示例

```cp
import * as std from "std";
import { BitSet } from "cap:bitset_simd";

fn main() -> ?void {
    const alloc = std.heap.page_allocator;
    let bs = BitSet.initEmpty(alloc, 256)?;
    defer bs.deinit();

    bs.setRange(60, 132)?;
    let total = bs.count(); // 72
    let first = bs.nextSetBit(0); // 60
}
```
