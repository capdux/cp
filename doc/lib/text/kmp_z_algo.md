# `cap:kmp_z_algo` — 纯 CP KMP / Z-Algorithm / Manacher 线性字符串算法库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:text`，导入请使用 `import { ... } from "cap:text"`。

## 概述

`cap:kmp_z_algo`（亦支持 `cap.kmp_z_algo` 动态挂载）实现三大经典线性时间 $O(N)$ 字符串算法：Knuth-Morris-Pratt (KMP) 前缀表与单/全重叠匹配、Z-Algorithm 前缀匹配数组 `zArray` 以及 Manacher 最长回文子串 `longestPalindrome`。

## 核心类型与 API

- `PalindromeSpan`: `{ start: usize, end: usize }`
- `computeLps(alloc, pattern) -> ![]usize`
- `kmpFindFirst(alloc, text, pattern) -> !usize?`
- `kmpFindAll(alloc, text, pattern) -> ![]usize`
- `zArray(alloc, s) -> ![]usize`
- `longestPalindrome(alloc, s) -> !PalindromeSpan`

## 示例

```cp
import { kmpFindAll, longestPalindrome } from "cap:text";

const hits = try kmpFindAll(alloc, "ababa", "aba");
defer alloc.free(hits); // [0, 2]
const pal = try longestPalindrome(alloc, "babad");
```
