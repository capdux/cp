[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **crates.io** [`fixedbitset`](https://github.com/petgraph/fixedbitset)（`MIT OR Apache-2.0`）（原库名 `fixedbitset`，CP 包名 `fixedbitset`）。

# fixedbitset

Zero-allocation fixed-size bitsets (`BitSet256` / `BitSet1024`) with SWAR popcount, zero-word-skipping scanning, and full set algebra for CP — clean-room MIT parity with Rust `fixedbitset` (634M), `bit-vec` (618M), and `bit-set` (557M).

纯 CP 零堆分配固定容量位集合引擎，适用于图算法、编译器活跃变量分析、调度器就绪队列与高频集合运算。

## Install / 安装

```bash
cap add fixedbitset
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `BitSet256` / `BitSet1024` | `init()` / `full()` / `clear(*_)` / `len() -> usize` | 256-bit (4×`u64`) and 1024-bit (16×`u64`) bitsets |
| `insert` / `remove` / `toggle` / `contains` | `bit: usize` | O(1) bounds-checked bit mutation & lookup |
| `setRange` | `(*_, start: usize, end: usize)` | Set half-open range `[start, end)` |
| `countOnes` / `isEmpty` | `() -> usize / bool` | SWAR popcount & emptiness check |
| `firstOne` / `nextOne` | `(from_bit: usize) -> usize?` | Fast word-skipping set-bit iterator |
| `unionWith` / `intersectWith` / `differenceWith` / `symmetricDiffWith` | `(*_, other)` | In-place set algebra |
| `isSubsetOf` / `isDisjoint` / `equals` | `(other) -> bool` | Set relation predicates |

## Usage / 使用示例

```cp
import { BitSet256 } from "fixedbitset";

let a = BitSet256.init();
a.setRange(10, 20);
* = a.insert(200);
const ones = a.countOnes(); // 11
const first = a.firstOne(); // 10
```
