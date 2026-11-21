# `cap:porter` — Martin Porter (1980) 英文词干提取器

零堆分配（Zero-Alloc）的纯 CP Martin Porter (1980) 经典英文词干提取算法（Porter Stemmer），严格实现原论文 5 步规则与 `[C](VC)^m[V]` 测度函数，适用于全文搜索引擎倒排索引归一化、文本聚类与信息检索（IR）。

## 核心特性

- **零堆分配**：基于栈上定长缓冲（支持最长 `PORTER_MAX_WORD_LEN = 256` 字节），无任何堆内存分配。
- **严格对标 Porter 1980 原论文**：完整实现 Step 1a/1b/1c、Step 2（20 条 `m > 0` 替换）、Step 3（7 条 `m > 0` 替换）、Step 4（19 条 `m > 1` 剥离，含 `*s`/`*t` 条件 `ion`）、Step 5a 与 Step 5b。
- **上下文 `'y'` 精确处理**：`porterMeasure` 与内部元辅音判定正确识别词首辅音 `'y'` 与辅音后元音 `'y'`。
- **严格输入边界防护**：长度 `<= 2` 短词原样小写返回；对空输入、非 ASCII 字母、超长词及输出缓冲不足统一返回 `null`。

## 导出 API

### 常量

- `PORTER_MAX_WORD_LEN: usize = 256`

### 函数

- `porterMeasure(stem: []const u8): usize`：计算小写 ASCII 词干的 Porter `[C](VC)^m[V]` 测度 `m`。
- `porterStem(word: []const u8, out: []u8): usize?`：将英文单词小写并执行完整 5 步词干提取，结果写入 `out` 并返回长度。
- `porterStemEquals(a: []const u8, b: []const u8): bool`：零分配判断两个英文单词是否具有相同词干。

## 使用示例

```cp
import std from "std";
import { porterMeasure, porterStem, porterStemEquals } from "cap:porter";

fn example() {
    * = porterMeasure("troubles"); // 2
    * = porterStemEquals("caresses", "caress"); // true
    * = porterStemEquals("relational", "relate"); // true

    let buf: [64]u8 = std.mem.zeroes([64]u8);
    if (n := porterStem("generalizations", buf[0..])) {
        // buf[0..n] == "gener"
        * = n;
    }
}
```
