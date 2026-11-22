# cap:suffix_array — 后缀数组与 Kasai LCP 标准库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:ds`，导入请使用 `import { ... } from "cap:ds"`。

## 1. 概述

`cap:suffix_array`（同时支持 `cap.suffix_array` 动态挂载）提供纯 CP 实现的工业级后缀数组（Suffix Array）、Kasai $O(N)$ 最长公共前缀（LCP）数组以及基于二分查找的高速子串检索能力。

- **构建复杂度**：$O(N \log N)$ 倍增计数排序构建后缀数组，$O(N)$ Kasai 算法构建 LCP 数组。
- **检索复杂度**：$O(M \log N)$ 二分上下界查找子串存在性、出现次数及全部起始位置（$M$ 为模式串长度）。
- **内存安全**：所有临时缓冲区在构建完成或异常退出时自动释放，实例内存通过 `deinit(&self)` 统一回收。

## 2. 核心类型

- `SuffixArray`：封装原始文本切片 `text`、后缀数组 `sa` 与 LCP 数组 `lcp` 的核心结构。

## 3. API 速查

| 方法签名 | 说明 |
| :--- | :--- |
| `SuffixArray.init(alloc: std.mem.Allocator, text: []const u8) -> !SuffixArray` | 构建文本的后缀数组与 Kasai LCP 数组 |
| `deinit(&self)` | 释放 `sa` 与 `lcp` 占用的内存 |
| `len(self) -> usize` | 返回文本字节长度 |
| `saSlice(self) -> []const usize` | 返回只读后缀数组切片 `sa` |
| `lcpSlice(self) -> []const usize` | 返回只读 LCP 数组切片 `lcp` |
| `contains(self, pattern: []const u8) -> bool` | 二分判断 `pattern` 是否存在（空模式串返回 `false`） |
| `countOccurrences(self, pattern: []const u8) -> usize` | 二分统计 `pattern` 出现的总次数 |
| `searchAll(self, alloc: std.mem.Allocator, pattern: []const u8) -> ![]usize` | 返回 `pattern` 所有出现位置的起始下标切片（需调用方 `alloc.free`） |
| `longestRepeatedSubstring(self) -> []const u8` | 基于 LCP 数组返回最长重复子串切片 |

## 4. 示例

```cp
import std from "std";
import { SuffixArray } from "cap:ds";

pub fn main() -> !void {
    const alloc = std.heap.page_allocator;
    let sa = try SuffixArray.init(alloc, "banana");
    defer sa.deinit();

    const lrs = sa.longestRepeatedSubstring(); // "ana"
    const count = sa.countOccurrences("ana");   // 2
    const positions = try sa.searchAll(alloc, "na");
    defer alloc.free(positions);                // [4, 2]
}
```
