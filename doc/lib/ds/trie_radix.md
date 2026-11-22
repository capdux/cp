# `cap:trie_radix` 路径压缩基数树标准库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:ds`，导入请使用 `import { ... } from "cap:ds"`。

## 概述

`cap:trie_radix`（支持 `import cap from "cap"` 下的 `cap.trie_radix` 动态挂载）提供纯 CP 实现的工业级路径压缩基数树（Compressed Radix Tree / Patricia Trie）。公共前缀边会被自动压缩为单条字节切片，并在插入分叉时自动分裂、在删除节点后自动合并单子节点压缩边，适用于路由表、最长前缀匹配、前缀自动补全与键空间索引等场景。

## 核心类型

- `StrSlice`：`[]const u8` 类型别名，用于前缀遍历返回的键切片列表。
- `PrefixMatch(comptime V: type)`：最长前缀匹配结果结构体，包含字段 `key: []const u8`（指向查询字符串前缀切片，零拷贝）与 `value: V`。
- `RadixTree(comptime V: type)`：泛型路径压缩基数树容器，内部以 `std.ArrayList` 维护压缩边子节点并在 `deinit` 时确定性释放全部节点与边内存。

## API 速查

| 方法 | 签名 | 说明 |
| :--- | :--- | :--- |
| `init` | `init(alloc: std.mem.Allocator) -> !RadixTree(V)` | 初始化空基数树 |
| `deinit` | `deinit(&self)` | 释放整棵树及全部压缩边内存 |
| `insert` | `insert(&self, key: []const u8, val: V) -> !V?` | 插入或覆盖键值对；返回被覆盖的旧值或 `null` |
| `get` | `get(self, key: []const u8) -> V?` | 精确查找键对应的值 |
| `remove` | `remove(&self, key: []const u8) -> V?` | 删除键并在无值且仅余单子节点时自动合并压缩边 |
| `longestPrefixMatch` | `longestPrefixMatch(self, query: []const u8) -> PrefixMatch(V)?` | 查找匹配 `query` 的最长前缀键值对 |
| `keysWithPrefix` | `keysWithPrefix(self, alloc: std.mem.Allocator, prefix: []const u8) -> ![]StrSlice` | 收集所有以 `prefix` 开头的键（调用方负责释放元素及切片） |
| `len` | `len(self) -> usize` | 返回当前存储的键值对数量 |

## 示例

```cp
import { RadixTree } from "cap:ds";
import std from "std";

pub fn main() -> !void {
    var tree = try RadixTree(u32).init(std.heap.page_allocator);
    defer tree.deinit();

    _ = try tree.insert("/api/v1/users", 100);
    _ = try tree.insert("/api/v1/posts", 200);

    if (m := tree.longestPrefixMatch("/api/v1/users/42")) {
        // m.key == "/api/v1/users", m.value == 100
        _ = m;
    }
}
```
