[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# diff-sequences

Zero-heap Longest Common Subsequence (LCS) length, diff insert/delete stats, LCS similarity, and Longest Common Substring locator.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:diff-sequences` ([https://github.com/jestjs/jest](https://github.com/jestjs/jest)) / `crates.io:lcs-diff` ([https://github.com/bokuweb/lcs-diff](https://github.com/bokuweb/lcs-diff))（原库名 `diff-sequences` → CP 包名 `diff-sequences`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `diff-sequences` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add diff-sequences
```

```cp
import {
    SubstringMatch,
    DiffStats,
    lcsLength,
    diffStats,
    lcsSimilarity,
    longestCommonSubstring,
} from "diff-sequences";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `SubstringMatch` | `class` | 字段: `a_start: usize, b_start: usize, len: usize` |
| `DiffStats` | `class` | 字段: `common: usize, insertions: usize, deletions: usize` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `lcsLength` | `(a: string, b: string): usize` | 调用 `lcsLength` 并返回 `usize`。 |
| `diffStats` | `(a: string, b: string): DiffStats` | 调用 `diffStats` 并返回 `DiffStats`。 |
| `lcsSimilarity` | `(a: string, b: string): f64` | 调用 `lcsSimilarity` 并返回 `f64`。 |
| `longestCommonSubstring` | `(a: string, b: string): SubstringMatch` | 调用 `longestCommonSubstring` 并返回 `SubstringMatch`。 |

## 最小调用示例

```cp
import { lcsLength } from "diff-sequences";

pub fn main() {
    let result = lcsLength("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
