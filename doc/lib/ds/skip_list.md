# `cap:skip_list` — 工业级有序跳表键值引擎

> [!NOTE]
> 本子模块已归并至统一主包 `cap:ds`，导入请使用 `import { ... } from "cap:ds"`。

## 概述

`cap:skip_list` 提供纯 CP 实现的工业级概率跳表（Skip List）有序键值存储引擎，支持 `import { SkipList } from "cap:ds"` 静态导入与 `import cap from "cap"` 下的 `cap.skip_list.SkipList` 动态挂载。内部采用确定性快速伪随机（`std.Random.DefaultPrng`）分配节点层高（`MAX_LEVEL = 16`），无外部 I/O 依赖，平均时间复杂度 $O(\log N)$。

## 核心类型

- `SkipList(comptime K: type, comptime V: type)`：泛型有序键值跳表容器。支持数值类型（整数、浮点数）及字符串切片（`[]const u8`）作为键 `K`。
- `SkipList(K, V).Entry`：键值对结构体 `struct { key: K, value: V }`。
- `SkipList(K, V).MAX_LEVEL`：最大跳表层高常量（`16`）。

## API 速查

| 方法 | 签名 | 说明 |
| --- | --- | --- |
| `init` | `init(alloc: std.mem.Allocator) -> Self` | 初始化空跳表 |
| `deinit` | `deinit(&self)` | 释放所有节点内存 |
| `clear` | `clear(&self)` | 清空所有节点并重置状态 |
| `insert` / `upsert` | `insert(&self, key: K, val: V) -> !V?` | 插入或覆盖更新键值，若键已存在则返回旧值 |
| `get` | `get(self, key: K) -> V?` | 精确查找键对应的值 |
| `contains` | `contains(self, key: K) -> bool` | 判断键是否存在 |
| `remove` | `remove(&self, key: K) -> V?` | 删除指定键并返回被删除的值 |
| `lowerBound` / `ceiling` | `lowerBound(self, key: K) -> Entry?` | 返回首个 `entry.key >= key` 的条目 |
| `floor` | `floor(self, key: K) -> Entry?` | 返回最大 `entry.key <= key` 的条目 |
| `rangeQuery` | `rangeQuery(self, alloc, start_key: K, end_key: K) -> ![]Entry` | 返回闭区间 `[start_key, end_key]` 内的有序条目切片 |
| `items` | `items(self, alloc) -> ![]Entry` | 返回全量有序条目切片 |
| `len` / `isEmpty` | `len(self) -> usize` / `isEmpty(self) -> bool` | 返回元素数量或判空 |

## 示例

```cp
import { SkipList } from "cap:ds";
const std = @import("std");

let sl = SkipList(i64, i64).init(std.heap.page_allocator);
defer sl.deinit();

* = try sl.insert(30, 300);
* = try sl.insert(10, 100);
* = try sl.insert(20, 200);

const lb = sl.lowerBound(15); // .{ .key = 20, .value = 200 }
const range = try sl.rangeQuery(std.heap.page_allocator, 10, 25);
defer std.heap.page_allocator.free(range);
```
