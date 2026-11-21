# `cap:suffix` — 零堆分配后缀数组与 Kasai LCP 字符串索引库

`cap:suffix` 提供纯 CP 实现、100% 零堆分配（zero-alloc）的后缀数组（Suffix Array）构建、Kasai 线性时间高度数组（LCP）、二分查找模式区间定位与子串统计分析工具，原生支持包含 `0x00` / `0xFF` 的任意二进制字节串。

## 核心特性

- **100% 零堆分配**：通过调用方提供的 `SuffixWorkspace` 完成倍增基数排序与 LCP 计算。
- **任意字节安全**：字节映射到 `1..256`、`0` 保留为越界哨兵，完整支持含空字节 `0x00` 与 `0xFF` 的二进制数据。
- **高效复杂度**：倍增 + 计数排序 $O(n \log n)$ 构建后缀数组；Kasai $O(n)$ 构建 LCP；两次二分查找 $O(|P| \log n)$ 定位模式串全部出现位置。
- **子串分析原语**：$O(n)$ 计算不同非空子串总数与最长重复子串。

---

## 导出清单

### 数据结构

- `SuffixWorkspace`：`{ rank: []usize, tmp_rank: []usize, cnt: []usize, order: []usize }`
  - 长度要求：`rank`/`tmp_rank`/`order` $\ge n$，`cnt` $\ge \max(n + 1, 257)$
- `OccurrenceRange`：`{ start: usize, end: usize, count: usize }`
- `LongestRepeat`：`{ pos: usize, len: usize }`

### 核心函数

| 函数签名 | 说明 |
| :--- | :--- |
| `buildSuffixArray(text: []const u8, sa_out: *[]usize, ws: *SuffixWorkspace): bool` | $O(n \log n)$ 构建后缀数组 `sa_out[0..n]` |
| `buildLcpArray(text: []const u8, sa: []const usize, lcp_out: *[]usize, rank_scratch: *[]usize): bool` | Kasai $O(n)$ 构建 LCP 数组（`lcp[0] = 0`） |
| `findPatternRange(text: []const u8, sa: []const usize, pattern: []const u8): OccurrenceRange` | $O(|P| \log n)$ 二分定位模式在 `sa` 中的半开区间 `[start, end)` |
| `countDistinctSubstrings(n: usize, lcp: []const usize): u64` | 精确计算不同非空子串总数 $n(n+1)/2 - \sum \text{lcp}$ |
| `longestRepeatedSubstring(sa: []const usize, lcp: []const usize): LongestRepeat` | 提取最长重复子串位置与长度 |

---

## 使用示例

```typescript
import std from "std";
import {
    SuffixWorkspace,
    buildSuffixArray,
    buildLcpArray,
    findPatternRange,
    countDistinctSubstrings,
    longestRepeatedSubstring,
} from "cap:suffix";

fn suffixDemo() {
    const text: []const u8 = "banana";
    let sa: [6]usize = std.mem.zeroes([6]usize);
    let lcp: [6]usize = std.mem.zeroes([6]usize);
    let rank: [6]usize = std.mem.zeroes([6]usize);
    let tmp_rank: [6]usize = std.mem.zeroes([6]usize);
    let order: [6]usize = std.mem.zeroes([6]usize);
    let cnt: [257]usize = std.mem.zeroes([257]usize);

    let ws = SuffixWorkspace {
        rank: rank[0..],
        tmp_rank: tmp_rank[0..],
        cnt: cnt[0..],
        order: order[0..],
    };
    let sa_s: []usize = sa[0..];
    let lcp_s: []usize = lcp[0..];
    let rk_s: []usize = rank[0..];

    * = buildSuffixArray(text, &sa_s, &ws);
    * = buildLcpArray(text, sa[0..], &lcp_s, &rk_s);

    const occ = findPatternRange(text, sa[0..], "ana"); // count == 2
    const distinct = countDistinctSubstrings(text.len, lcp[0..]); // 15
    const lrs = longestRepeatedSubstring(sa[0..], lcp[0..]); // len == 3 ("ana")
    * = occ;
    * = distinct;
    * = lrs;
}
```
