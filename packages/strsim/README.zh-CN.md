[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# strsim

Zero-heap string similarity & edit-distance metrics: Hamming, Levenshtein, OSA Damerau-Levenshtein, Jaro-Winkler, and Sorensen-Dice (clean-room port of Rust strsim)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:strsim` ([https://github.com/rapidfuzz/strsim-rs](https://github.com/rapidfuzz/strsim-rs))

| 元信息 | 值 |
| --- | --- |
| 包名 | `strsim` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add strsim
```

```cp
import {
    ssmMin3,
    hamming,
    levenshtein,
    osaDistance,
    normalizedLevenshteinBps,
    jaroBps,
    jaroWinklerBps,
    sorensenDiceBps,
} from "strsim";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `ssmMin3` | `(a: usize, b: usize, c: usize): usize` | 调用 `ssmMin3` 并返回 `usize`。 |
| `hamming` | `(a: string, b: string): usize?` | 调用 `hamming` 并返回 `usize?`。 |
| `levenshtein` | `(a: string, b: string): usize` | 调用 `levenshtein` 并返回 `usize`。 |
| `osaDistance` | `(a: string, b: string): usize` | 调用 `osaDistance` 并返回 `usize`。 |
| `normalizedLevenshteinBps` | `(a: string, b: string): u32` | 调用 `normalizedLevenshteinBps` 并返回 `u32`。 |
| `jaroBps` | `(a: string, b: string): u32` | 调用 `jaroBps` 并返回 `u32`。 |
| `jaroWinklerBps` | `(a: string, b: string): u32` | 调用 `jaroWinklerBps` 并返回 `u32`。 |
| `sorensenDiceBps` | `(a: string, b: string): u32` | 调用 `sorensenDiceBps` 并返回 `u32`。 |

## 最小调用示例

```cp
import { hamming } from "strsim";

pub fn main() {
    let result = hamming("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
