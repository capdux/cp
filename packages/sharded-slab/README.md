[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/slab

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:sharded-slab` ([https://github.com/hawkw/sharded-slab](https://github.com/hawkw/sharded-slab))（原库名 `sharded-slab` → CP 包名 `@cap/slab`）

Zero-allocation O(1) generational slab allocator and ABA-safe handle pool for CP — clean-room MIT parity with Rust `slab` (1.19B downloads on crates.io) + `generational-arena`.

纯 CP 零堆分配 O(1) 侵入式空闲链表对象池与防悬垂代数句柄引擎，适用于连接池、定时器句柄、事件源与资源表映射。

## Install / 安装

```bash
cap add @cap/slab
```

## Features / 核心特性

- **O(1) Allocation & Free**: Intrusive free-list + bump pointer over fixed 256-slot stack storage (`SlabI64`, `SlabU64`), zero heap allocation.
- **ABA / Use-After-Free Protection**: Every `SlabKey{ index, generation }` carries a generation counter that increments on `remove()` and `clear()`, invalidating stale handles even after slot reuse.

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `SlabKey` | `struct { index: u32, generation: u32 }` | Generational handle (`equals(other) -> bool`) |
| `SlabI64` / `SlabU64` | `init() -> Slab*` | Initialize empty 256-slot slab |
| `insert` | `(*_, value) -> SlabKey?` | O(1) insert (reuses freed slot or bumps); returns `null` when full |
| `get` | `(key: SlabKey) -> T?` | O(1) lookup with generation verification |
| `set` | `(*_, key: SlabKey, value) -> bool` | O(1) update if handle is live |
| `remove` | `(*_, key: SlabKey) -> T?` | O(1) remove, bump generation, push slot onto free list |
| `contains` | `(key: SlabKey) -> bool` | Check if handle is currently live |
| `count` / `isEmpty` / `isFull` | `() -> usize / bool` | Occupancy queries |
| `clear` | `(*_)` | Clear all live entries and bump their generations |

## Usage / 使用示例

```cp
import { SlabI64, SlabKey } from "@cap/slab";

let pool = SlabI64.init();
const k1 = pool.insert(42 as i64) ?? SlabKey{ index: 0 as u32, generation: 0 as u32 };
const v1 = pool.get(k1); // 42
* = pool.remove(k1);

const k2 = pool.insert(99 as i64) ?? SlabKey{ index: 0 as u32, generation: 0 as u32 };
// k2 reuses index 0 with generation 2; pool.get(k1) safely returns null!
```
