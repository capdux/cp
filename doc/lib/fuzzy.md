// Source: cp/doc/lib/fuzzy.md
# `cap:fuzzy` — 模糊字符串匹配、编辑距离、Jaro-Winkler 相似度与排序检索

纯 CP、零 C 依赖的高性能模糊字符串匹配与编辑距离标准库，提供 $O(\min(M, N))$ 空间优化的 Unicode Levenshtein 距离、支持相邻换位的真 Damerau-Levenshtein 距离（Lowrance-Wagner）、带前缀缩放校验的 Jaro / Jaro-Winkler 相似度，以及面向命令面板与路径检索的 fzf 风格子序列打分器（支持精确命中下标提取、词边界/驼峰/连续命中加分与候选排序）。

## 导入方式

```cp
import {
    FuzzyError,
    FuzzyMatch,
    levenshtein,
    damerauLevenshtein,
    jaro,
    jaroWinkler,
    fuzzyScore,
    rankCandidates,
    freeMatches,
} from "cap:fuzzy";

// 或动态命名空间挂载：
import cap from "cap";
// cap.fuzzy.levenshtein / cap.fuzzy.jaroWinkler / cap.fuzzy.fuzzyScore / cap.fuzzy.rankCandidates
```

## 核心类型

### `FuzzyError`
- `InvalidPrefixScale`：`jaroWinkler` 的 `prefix_scale` 不在 `[0.0, 0.25]` 合法区间内（或为 `NaN`）
- `OutOfMemory`：内存分配失败

### `FuzzyMatch`
- `candidate: []const u8`：命中的候选字符串切片
- `index: usize`：在候选数组中的 0 基原始索引（`fuzzyScore` 单独调用时为 `0`）
- `score: i64`：fzf 风格综合匹配得分
- `positions: []usize`：与 `query` 等长的精确命中字节下标切片
- `deinit(*_, allocator: std.mem.Allocator)`：释放 `positions` 堆内存

## 核心 API

### 编辑距离与相似度原语
- `levenshtein(allocator, a: []const u8, b: []const u8): usize ! FuzzyError`：基于 Unicode 码点的 $O(\min(M, N))$ 空间优化 Levenshtein 编辑距离
- `damerauLevenshtein(allocator, a: []const u8, b: []const u8): usize ! FuzzyError`：基于 Unicode 码点的无限制 Damerau-Levenshtein 编辑距离（相邻换位代价为 1）
- `jaro(allocator, a: []const u8, b: []const u8): f64 ! FuzzyError`：标准 Jaro 相似度（`0.0..1.0`）
- `jaroWinkler(allocator, a: []const u8, b: []const u8, prefix_scale: f64): f64 ! FuzzyError`：Jaro-Winkler 相似度（校验 `0.0 <= prefix_scale <= 0.25`，越界返回 `FuzzyError.InvalidPrefixScale`）

### fzf 风格模糊子序列打分与排序检索
- `fuzzyScore(allocator, query: []const u8, target: []const u8, case_sensitive: bool): (FuzzyMatch?) ! FuzzyError`：顺序子序列动态规划匹配；对连续命中、词边界（`/` `-` `_` `.` 空格）、驼峰大写首字母及首字符命中给予加分，对前导偏移与间隙进行惩罚，并输出精确命中下标 `positions`
- `rankCandidates(allocator, query: []const u8, candidates: []const []const u8, case_sensitive: bool): ([]FuzzyMatch) ! FuzzyError`：过滤未命中候选并按 `score` 降序、原始 `index` 升序排列
- `freeMatches(allocator, matches: []FuzzyMatch)`：释放 `rankCandidates` 返回的切片及其中每个 `FuzzyMatch` 的 `positions`
