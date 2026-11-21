# `cap:kmp` — 零分配线性时间字符串匹配与回文算法库（KMP / Z-Algorithm / Manacher）

零堆分配（Zero-Alloc）的纯 CP 线性时间字节串算法库，集成三大经典 O(n) 算法：Knuth-Morris-Pratt（KMP）前缀函数与重叠匹配、Z-Algorithm 最长公共前缀数组（Z-Array），以及无需哨兵字符的 Manacher 最长回文子串与回文计数算法（完全兼容含 `\x00`..`\xff` 的任意二进制数据）。

## 核心特性

- **零堆分配（Zero-Alloc）**：全部前缀表 `pi`、Z 数组 `z`、回文半径 `d1`/`d2` 与匹配位置输出切片均由调用方栈或静态缓冲提供。
- **KMP 全套能力**：支持最长真前后缀计算 `computePrefixFunction`、首次匹配 `kmpFindFirst`、全量重叠匹配 `kmpFindAll`、零输出缓冲计数 `kmpCount` 与最小周期 `shortestPeriod`。
- **O(n) Z 函数**：`computeZArray` 精确计算 `z[i] = lcp(s, s[i..])`（含 `z[0] == s.len`）。
- **无哨兵 Manacher**：分别在线性时间内计算奇数半径 `d1` 与偶数半径 `d2`，不插入任何哨兵字节，天然支持任意二进制流。

## 导出 API

### 数据模型

- `PalindromeSpan { start: usize, len: usize }`

### KMP 算法

- `computePrefixFunction(pattern: []const u8, pi_out: []usize): bool`
- `kmpFindFirst(text: []const u8, pattern: []const u8, pi: []const usize): usize?`
- `kmpFindAll(text: []const u8, pattern: []const u8, pi: []const usize, matches_out: []usize): usize`
- `kmpCount(text: []const u8, pattern: []const u8, pi: []const usize): usize`
- `shortestPeriod(s: []const u8, pi: []const usize): usize`

### Z-Algorithm

- `computeZArray(s: []const u8, z_out: []usize): bool`

### Manacher 回文算法

- `manacherLongestPalindrome(s: []const u8, d1_scratch: []usize, d2_scratch: []usize): PalindromeSpan?`
- `countPalindromicSubstrings(s: []const u8, d1_scratch: []usize, d2_scratch: []usize): usize?`

## 使用示例

```cp
import std from "std";
import {
    computePrefixFunction,
    kmpFindAll,
    kmpCount,
    shortestPeriod,
    computeZArray,
    manacherLongestPalindrome,
} from "cap:kmp";

fn example() {
    const text = "aaaaa";
    const pat = "aaa";
    let pi: [3]usize = std.mem.zeroes([3]usize);
    * = computePrefixFunction(pat, pi[0..]);

    let matches: [8]usize = std.mem.zeroes([8]usize);
    * = kmpFindAll(text, pat, pi[0..], matches[0..]); // 3 -> [0, 1, 2]
    * = kmpCount(text, pat, pi[0..]);                 // 3

    const s = "abcabcabc";
    let pi_s: [9]usize = std.mem.zeroes([9]usize);
    * = computePrefixFunction(s, pi_s[0..]);
    * = shortestPeriod(s, pi_s[0..]);                 // 3

    let z: [9]usize = std.mem.zeroes([9]usize);
    * = computeZArray(s, z[0..]);

    const pal = "xabbay";
    let d1: [6]usize = std.mem.zeroes([6]usize);
    let d2: [6]usize = std.mem.zeroes([6]usize);
    * = manacherLongestPalindrome(pal, d1[0..], d2[0..]); // { start: 1, len: 4 }
}
```
