[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# fastest-levenshtein

Zero-heap Levenshtein edit distance, OSA Damerau-Levenshtein transposition distance, normalized similarity, and closest-match lookup.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:fastest-levenshtein` ([https://github.com/ka-weihe/fastest-levenshtein](https://github.com/ka-weihe/fastest-levenshtein)) / `crates.io:levenshtein` ([https://github.com/wooorm/levenshtein-rs](https://github.com/wooorm/levenshtein-rs))（原库名 `fastest-levenshtein` → CP 包名 `fastest-levenshtein`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `fastest-levenshtein` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add fastest-levenshtein
```

```cp
import {
    absDiff,
    min3,
    distance,
    damerauDistance,
    similarity,
    withinDistance,
    closestIndex,
} from "fastest-levenshtein";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `absDiff` | `(a: usize, b: usize): usize` | 调用 `absDiff` 并返回 `usize`。 |
| `min3` | `(a: usize, b: usize, c: usize): usize` | 调用 `min3` 并返回 `usize`。 |
| `distance` | `(a: string, b: string): usize` | 调用 `distance` 并返回 `usize`。 |
| `damerauDistance` | `(a: string, b: string): usize` | 调用 `damerauDistance` 并返回 `usize`。 |
| `similarity` | `(a: string, b: string): f64` | 调用 `similarity` 并返回 `f64`。 |
| `withinDistance` | `(a: string, b: string, max_dist: usize): bool` | 调用 `withinDistance` 并返回 `bool`。 |
| `closestIndex` | `(query: string, candidates: &[]string, max_dist: usize): i64` | 释放模块分配的底层资源。 |

## 最小调用示例

```cp
import { distance } from "fastest-levenshtein";

pub fn main() {
    let result = distance("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
