# `cap:bptree` — 工业级内存有序索引 B+ 树

## 概述

`cap:bptree` 是纯 CP 实现的高性能内存有序索引标准库，基于固定阶数（`MAX_KEYS = 16`、`MIN_KEYS = 8`）的 B+ 树（B+ Tree）结构。所有键值对均有序存储于叶子节点中，且叶子节点之间通过单向链表串联，支持高效的点查、更新（Upsert）、删除（含借位与合并再平衡）以及区间范围扫描（`rangeScan`）。

## 核心类型

- `BPlusTree(comptime K: type, comptime V: type): type` — 泛型 B+ 树索引容器。
- `BPlusTree(K, V).Entry` — 范围扫描返回的键值对结构体：
  - `key: K`
  - `value: V`

## API 速查

| 方法 | 签名 | 说明 |
| :--- | :--- | :--- |
| `init` | `init(alloc: std.mem.Allocator): Self` | 初始化空 B+ 树 |
| `deinit` | `deinit(&self)` | 递归释放全部内部节点与叶子节点 |
| `insert` | `insert(&self, key: K, val: V): !V?` | 插入或更新键值对；若键已存在则覆盖并返回旧值 |
| `get` | `get(self, key: K): V?` | 二分查找指定键，未命中返回 `null` |
| `remove` | `remove(&self, key: K): V?` | 删除指定键并自动执行借位/合并再平衡，返回被删值 |
| `rangeScan` | `rangeScan(self, alloc: std.mem.Allocator, min_key: K, max_key: K): ![]Entry` | 利用叶子链表扫描 `[min_key, max_key]` 闭区间并按序返回切片（调用方负责 `alloc.free`） |
| `len` | `len(self): usize` | 返回当前存储的键值对总数 |

## 示例

```cp
import { BPlusTree } from "cap:bptree";
import std from "std";

pub fn main(): !void {
    let tree = BPlusTree(i64, i64).init(std.heap.page_allocator);
    defer tree.deinit();

    * = try tree.insert(10, 100);
    * = try tree.insert(5, 50);
    * = try tree.insert(20, 200);

    const val = tree.get(10); // 100
    const entries = try tree.rangeScan(std.heap.page_allocator, 5, 15);
    defer std.heap.page_allocator.free(entries);
    * = val;
}
```
