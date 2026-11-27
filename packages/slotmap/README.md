[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/slotmap

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:slotmap` ([https://github.com/orlp/slotmap](https://github.com/orlp/slotmap))

Zero-allocation generational-index slot map (`SlotKey`, `SlotMapI64`, `SecondaryMapI64`) with O(1) free-list and strict ABA dangling-key protection for CP — clean-room MIT parity with Rust `slotmap` v1 (550M+ downloads).

纯 CP 零堆分配代际索引安全槽位容器，适用于 ECS 实体管理、图节点/边索引、连接句柄表与无锁/固定内存资源池。

## Install / 安装

```bash
cap add @cap/slotmap
```

## Design / 设计原理

- **代际版本号（Generational Version）**：每个槽位维护一个 `u32` 版本号——**奇数表示已占用，偶数表示空闲**。插入时版本号 +1（偶 -> 奇），删除或 `clear()` 时版本号 +1（奇 -> 偶）并挂回空闲链表，彻底杜绝 ABA 悬垂键误读/误删。
- **零堆分配与 O(1) 空闲链表**：固定 128 槽位，内部维护侵入式 `next_free` 索引链表，`insert` / `get` / `set` / `remove` / `containsKey` 均为严格 O(1)。
- **二级属性表（`SecondaryMapI64`）**：与主表生成的 `SlotKey` 精确配对，支持为同一组实体挂载额外属性且同样具备代际防 ABA 校验。

## API

### `SlotKey`

| Method | Signature | Description |
| --- | --- | --- |
| `nullKey` | `static fn nullKey() -> SlotKey` | Returns the null sentinel key (`idx: 0, version: 0`) |
| `isNull` | `fn isNull() -> bool` | Returns `true` when `version == 0` |
| `eq` | `fn eq(other: SlotKey) -> bool` | Compares both `idx` and `version` |
| `toU64` | `fn toU64() -> u64` | Packs `(version << 32) \| idx` into a `u64` |
| `fromU64` | `static fn fromU64(raw: u64) -> SlotKey` | Unpacks a `u64` back into `SlotKey` |

### `SlotMapI64`

| Method | Signature | Description |
| --- | --- | --- |
| `new` | `static fn new() -> SlotMapI64` | Creates an empty 128-slot map |
| `len` / `capacity` / `isEmpty` | `() -> usize / usize / bool` | Query occupancy and fixed capacity (128) |
| `insert` | `(*_, val: i64) -> SlotKey?` | Inserts value in O(1); returns `null` when full |
| `get` | `(key: SlotKey) -> i64?` | Looks up live key; returns `null` for stale/invalid keys |
| `set` | `(*_, key: SlotKey, val: i64) -> bool` | Updates live key in place |
| `remove` | `(*_, key: SlotKey) -> i64?` | Removes live key, increments version, pushes slot to free-list |
| `containsKey` | `(key: SlotKey) -> bool` | Checks whether `key` refers to a live slot |
| `clear` | `(*_)` | Invalidates all live slots (bumping versions) and resets free-list |

### `SecondaryMapI64`

| Method | Signature | Description |
| --- | --- | --- |
| `new` | `static fn new() -> SecondaryMapI64` | Creates an empty 128-slot secondary map |
| `insert` | `(*_, key: SlotKey, val: i64) -> bool` | Associates `val` with a valid occupied `SlotKey` |
| `get` | `(key: SlotKey) -> i64?` | Fetches value matching exact `(idx, version)` |
| `remove` | `(*_, key: SlotKey) -> i64?` | Removes matching entry and returns old value |
| `containsKey` | `(key: SlotKey) -> bool` | Checks whether `key` has an entry |
| `len` | `() -> usize` | Returns number of stored entries |
| `clear` | `(*_)` | Clears all entries |

## Usage / 使用示例

```cp
import { SlotMapI64, SecondaryMapI64 } from "@cap/slotmap";

let entities = SlotMapI64.new();
let hp = SecondaryMapI64.new();

let k1 = entities.insert(100 as i64) ?? SlotKey.nullKey();
* = hp.insert(k1, 250 as i64);

const val = entities.get(k1); // 100
const old = entities.remove(k1); // 100
const stale = entities.get(k1); // null (ABA protected)
```
