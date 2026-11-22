// Source: cp/doc/lib/suffixarray.md
# `cap:suffixarray` — 后缀数组、Kasai LCP 与子串索引引擎

> [!NOTE]
> 本子模块已归并至统一主包 `cap:text`，导入请使用 `import { ... } from "cap:text"`。

纯 CP、零 C 依赖的高性能后缀数组（Suffix Array）与 Kasai 最长公共前缀（LCP）子串索引引擎。采用倍增前缀排序 + 基数（计数）排序在 $O(N \log N)$ 时间内构建后缀数组，配合 Kasai $O(N)$ 算法构建 LCP 数组，支持 $O(|P| \log N)$ 上下界二分查找、全匹配偏移提取、$O(N)$ 最长重复子串提取与不同子串计数。

## 导入方式

```cp
import {
    SuffixArrayError,
    SuffixArray,
} from "cap:text";

// 或动态命名空间挂载：
import cap from "cap";
// cap.suffixarray.SuffixArray / cap.suffixarray.SuffixArrayError
```

## 核心类型

### `SuffixArrayError`
- `EmptyPattern`：传入空模式串（`pattern.len == 0`）
- `OutOfMemory`：内存分配失败

## 核心 API

### `SuffixArray`
- `SuffixArray.build(alloc: std.mem.Allocator, text: []const u8): SuffixArray ! SuffixArrayError`
  - 使用倍增前缀排序 + 基数排序在 $O(N \log N)$ 构建后缀数组 `sa` 与 Kasai $O(N)$ `lcp` 数组。
  - 支持空文本（`text.len == 0`）、单字节与包含 `\x00` 的任意二进制载荷。
  - 内部拷贝 `text` 持有独立所有权，调用 `deinit()` 统一释放。
- `deinit(*_)`: 释放内部 `text`、`sa` 与 `lcp` 缓冲区。
- `contains(pattern: []const u8): bool ! SuffixArrayError`
  - 利用 `sa` 二分查找判断模式串是否存在（$O(|P| \log N)$）；空模式返回 `SuffixArrayError.EmptyPattern`。
- `count(pattern: []const u8): usize ! SuffixArrayError`
  - 利用上下界二分查找统计命中总次数（含重叠出现，$O(|P| \log N)$）；空模式返回 `SuffixArrayError.EmptyPattern`。
- `findAll(alloc: std.mem.Allocator, pattern: []const u8): ([]usize) ! SuffixArrayError`
  - 返回全部匹配起始字节偏移（升序排列）；调用方负责 `alloc.free(result)`；空模式返回 `SuffixArrayError.EmptyPattern`。
- `longestRepeatedSubstring(): []const u8`
  - 基于 `lcp` 最大值在 $O(N)$ 时间内提取最长重复子串（指向内部 `text` 的只读切片）；无重复子串时返回空切片。
- `distinctSubstringCount(): u64`
  - 基于公式 $\frac{N(N+1)}{2} - \sum_{i=0}^{N-1} LCP[i]$ 在 $O(N)$ 时间内计算不同非空子串总数。
- `saSlice(): []const usize` / `lcpSlice(): []const usize` / `len(): usize`
  - 返回后缀数组只读切片、Kasai LCP 数组只读切片与文本字节长度。
