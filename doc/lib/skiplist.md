# CP 标准库：纯 CP 有序跳表 MemTable 与范围扫描引擎 (`cap:skiplist`)

## 一、 架构总览与设计准则

`cap:skiplist` 是 CP 语言官方内置的纯 CP **泛型有序跳表 MemTable 与范围扫描索引（Ordered SkipList MemTable & Range Scan Index）**，**零 C 依赖、零 `std.posix`、零 `@import("bun")`**。

核心特性：
- **确定性 SplitMix64 层高生成**：内置 `SplitMix64` 伪随机层高生成器，支持 `1..=32` 层指针塔（默认 `max_level = 16`），无需外部系统随机源。
- **键所有权安全**：`upsert` 内部对 `[]const u8` 键执行深拷贝（`allocator.dupe`），彻底杜绝调用方缓冲区复用或释放造成的悬垂指针。
- **O(log N) 点查与范围扫描**：支持 $O(\log N)$ 期望时间复杂度的 `upsert`、`get`、`contains`、`removeKey`、`lowerBound`，以及 $O(1)$ 的 `first` / `last` 边界查询与严格字典序半开区间 `[start_inclusive, end_exclusive)` `rangeScan`。

既可通过 `import { SkipList, SkipEntry, SkipListError } from "cap:skiplist"` 导入，也支持 `import cap from "cap"; cap.skiplist.*` 动态懒挂载调用。

### 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/skiplist/skiplist.cp` | `// Source: cp/lib/skiplist/skiplist.cp (...)` | `SkipListError`、`SkipEntry<V>`、`SkipList<V>` 核心实现 |
| `lib/skiplist/index.cp` | `// Source: cp/lib/skiplist/index.cp (...)` | `cap:skiplist` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集与条目类型

- **`SkipListError`**：
  - `InvalidMaxLevel`：初始化传入 `max_level == 0` 或 `max_level > 32`。
  - `InvalidRange`：`rangeScan` 传入 `start_inclusive > end_exclusive`。
  - `OutOfMemory`：节点、前驱指针塔、键拷贝或扫描输出切片分配失败。
- **`SkipEntry<V>`**：
  - `{ key: []const u8, value: V }`：只读键值条目视图（`key` 生命周期与对应键在跳表中未被删除或清空前一致）。

### 2. `SkipList<V>` 核心方法

- **生命周期**：
  - `SkipList<V>.init(allocator: std.mem.Allocator): SkipList<V> ! SkipListError`：以默认 `max_level = 16` 与默认 SplitMix64 种子初始化跳表。
  - `SkipList<V>.initWithSeed(allocator: std.mem.Allocator, max_level: usize, seed: u64): SkipList<V> ! SkipListError`：指定最大层高（`1..=32`）与确定性随机数种子初始化跳表。
  - `deinit(*_)`：释放全部节点、内部拷贝的键内存、层高指针数组及哨兵头节点。
- **写入与删除**：
  - `upsert(*_, key: []const u8, value: V): bool ! SkipListError`：插入或覆盖更新键值对。若 `key` 为新增则返回 `true`；若 `key` 已存在则覆盖其值并返回 `false`。
  - `removeKey(*_, key: []const u8): V?`：删除指定键，更新各层前驱指针并安全释放节点与键内存，返回被删除的旧值（不存在返回 `null`）。
  - `clear(*_)`：清空全部条目并释放节点与键内存，保留哨兵头节点以便复用。
- **点查与边界定位**：
  - `get(key: []const u8): V?`：$O(\log N)$ 精确点查。
  - `contains(key: []const u8): bool`：判断键是否存在。
  - `lowerBound(key: []const u8): SkipEntry<V>?`：$O(\log N)$ 返回首个满足 `entry.key >= key` 的条目。
  - `first(): SkipEntry<V>?`：返回字典序最小条目，空表返回 `null`。
  - `last(): SkipEntry<V>?`：$O(1)$ 返回字典序最大条目，空表返回 `null`。
- **范围扫描与状态**：
  - `rangeScan(allocator: std.mem.Allocator, start_inclusive: []const u8, end_exclusive: []const u8): []SkipEntry<V> ! SkipListError`：按严格字典序返回 `[start_inclusive, end_exclusive)` 区间内的全部条目；若 `start_inclusive > end_exclusive` 返回 `SkipListError.InvalidRange`。调用方需使用 `allocator.free(entries)` 释放返回切片。
  - `len(): usize`：返回当前键值对数量。
  - `isEmpty(): bool`：返回跳表是否为空。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { SkipList, SkipEntry, SkipListError } from "cap:skiplist";
import cap from "cap";

pub fn demoSkipList(): ! {
    const alloc = std.heap.page_allocator;

    let sl = SkipList<i64>.init(alloc)?;
    defer sl.deinit();

    * = sl.upsert("k:03", 30)?;
    * = sl.upsert("k:01", 10)?;
    * = sl.upsert("k:02", 20)?;

    const items = sl.rangeScan(alloc, "k:01", "k:03")?;
    defer alloc.free(items); // 返回 k:01, k:02

    let dyn_sl = cap.skiplist.SkipList<i32>.initWithSeed(alloc, 32, 42)?;
    defer dyn_sl.deinit();
    * = dyn_sl.upsert("alpha", 1)?;
}
```
