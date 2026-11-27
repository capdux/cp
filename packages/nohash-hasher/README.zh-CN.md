[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# nohash-hasher

纯 CP 实现的零开销整数恒等哈希器、Fibonacci/SplitMix64 整数混合器以及固定容量栈上哈希容器（`IntMap64`、`IntSet64`）。

- **源包对齐**：对标 `crates.io` `nohash-hasher` `v0.2.0`（Apache-2.0）
- **包版本**：`0.2.0`
- **内存模型**：全程零堆分配，支持无开销位转换恒等哈希与 64 槽位栈上开放寻址表。

## 安装与导入

```bash
cap add nohash-hasher
```

```cp
import {
    INT_TABLE_CAP,
    IntHasher,
    NoHashHasher,
    BuildNoHashHasher,
    newIntHasher,
    newNoHashHasher,
    nohashU64,
    nohashI32,
    fibBucketIndex,
    mixU64,
    unmixU64,
    IntMap64,
    IntSet64,
} from "nohash-hasher";
```

## 核心 API

| 符号 | 签名 | 说明 |
| --- | --- | --- |
| `INT_TABLE_CAP` | `const INT_TABLE_CAP: usize` | `IntMap64` 与 `IntSet64` 的固定槽位容量（`64`）。 |
| `IntHasher` / `NoHashHasher` | `class IntHasher { state: u64, ... }` | 恒等哈希器，提供 `static fn new()`、`writeU8..writeUsize`、`writeI8..writeIsize`、`finish(): u64`、`reset()`、`clone()`。 |
| `BuildNoHashHasher` | `class BuildNoHashHasher { seed: u64, ... }` | 哈希构建器，提供 `static fn new()` 与 `buildHasher(): IntHasher`。 |
| `newIntHasher` / `newNoHashHasher` | `fn newIntHasher(): IntHasher` | 独立函数构造器，返回 `IntHasher` / `NoHashHasher`。 |
| `nohashU8`..`nohashUsize` | `fn nohashU64(key: u64): u64`（含 `U8`/`U16`/`U32`/`Usize`） | 一次性无符号整数恒等哈希函数。 |
| `nohashI8`..`nohashIsize` | `fn nohashI64(key: i64): u64`（含 `I8`/`I16`/`I32`/`Isize`） | 一次性有符号整数位转换恒等哈希函数。 |
| `fibBucketIndex` | `fn fibBucketIndex(key: u64, bits: u6): usize` | Fibonacci 乘法桶索引映射，输出范围 `0..(1 << bits)`。 |
| `mixU64` / `unmixU64` | `fn mixU64(x: u64): u64` / `fn unmixU64(x: u64): u64` | 双射 SplitMix64 雪崩混合函数及其精确逆函数。 |
| `IntMap64` | `class IntMap64 { ... }` | 栈上 64 槽位 `u64 -> u64` 映射：`init()`、`len()`、`isEmpty()`、`clear()`、`insert(key, val): bool`、`get(key): u64?`、`containsKey(key): bool`、`remove(key): u64?`。 |
| `IntSet64` | `class IntSet64 { ... }` | 栈上 64 槽位 `u64` 集合：`init()`、`len()`、`isEmpty()`、`clear()`、`insert(key): bool`、`contains(key): bool`、`remove(key): bool`。 |

## 使用示例

### 1. 恒等哈希与可逆混合

```cp
import { IntHasher, nohashU64, mixU64, unmixU64, fibBucketIndex } from "nohash-hasher";

fn hashDemo(): void {
    let mut h = IntHasher.new();
    h.writeU64(42);
    const digest = h.finish(); // 42

    const direct = nohashU64(42); // 42
    const mixed = mixU64(42);
    const orig = unmixU64(mixed); // 42
    const bucket = fibBucketIndex(42, 6); // 0..63
}
```

### 2. 零堆分配 `IntMap64` 与 `IntSet64`

```cp
import { IntMap64, IntSet64 } from "nohash-hasher";

fn containerDemo(): void {
    let mut map = IntMap64.init();
    map.insert(1001, 99);
    const v = map.get(1001);       // Some(99)
    const old = map.remove(1001);  // Some(99)

    let mut set = IntSet64.init();
    set.insert(7);
    const has7 = set.contains(7);  // true
}
```

## 与源包差异与安全亮点

- **显式类型方法**：无需 Rust `Hasher` trait 动态分发，直接提供 `writeU8..writeIsize` 与 `nohash*` 内联函数。
- **内置栈容器**：自带 64 槽位的 `IntMap64` 与 `IntSet64`，适合无 GC、零堆分配的小规模整数索引表。
- **可逆混合增强**：额外提供 SplitMix64 `mixU64`/`unmixU64` 与 `fibBucketIndex`，在保留低开销的同时避免连续键聚簇。
