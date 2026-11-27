[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# pep440_rs

Zero-heap PEP 440 Python version parser, total-order comparator, and specifier matcher in pure CP (modeled after Astral pep440_rs)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:pep440_rs` ([https://github.com/astral-sh/uv](https://github.com/astral-sh/uv))（原库名 `pep440_rs` → CP 包名 `pep440_rs`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `pep440_rs` |
| 版本 | `1.0.0` |
| 许可证 | `MIT OR Apache-2.0` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add pep440_rs
```

```cp
import {
    PreKind,
    Pep440Version,
    PepNumRes,
    pepInvalid,
    pepIsDigit,
    pepIsSpace,
    pepToLower,
    pepIsSep,
} from "pep440_rs";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `PreKind` | `None, Alpha, Beta, Rc` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Pep440Version` | `class` | 字段: `valid: bool, epoch: u32, release: [4]u32, release_len: u8, pre_kind: PreKind, pre_num: u32, +4 项` |
| `PepNumRes` | `class` | 字段: `ok: bool, val: u32, next: usize` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `pepInvalid` | `(): Pep440Version` | 调用 `pepInvalid` 并返回 `Pep440Version`。 |
| `pepIsDigit` | `(c: u8): bool` | 调用 `pepIsDigit` 并返回 `bool`。 |
| `pepIsSpace` | `(c: u8): bool` | 调用 `pepIsSpace` 并返回 `bool`。 |
| `pepToLower` | `(c: u8): u8` | 调用 `pepToLower` 并返回 `u8`。 |
| `pepIsSep` | `(c: u8): bool` | 调用 `pepIsSep` 并返回 `bool`。 |
| `pepMatchWord` | `(s: string, pos: usize, end: usize, word: string): bool` | 调用 `pepMatchWord` 并返回 `bool`。 |
| `pepReadU32` | `(s: string, pos: usize, end: usize): PepNumRes` | 调用 `pepReadU32` 并返回 `PepNumRes`。 |
| `parsePep440` | `(s: string): Pep440Version` | 通过 `parsePep440` 解析输入数据，返回 `Pep440Version`。 |
| `pepCmpU32` | `(a: u32, b: u32): i32` | 调用 `pepCmpU32` 并返回 `i32`。 |
| `pepPreRank` | `(k: PreKind): i32` | 调用 `pepPreRank` 并返回 `i32`。 |
| `pepStageRank` | `(v: Pep440Version): i32` | 调用 `pepStageRank` 并返回 `i32`。 |
| `comparePep440` | `(a: Pep440Version, b: Pep440Version): i32` | 调用 `comparePep440` 并返回 `i32`。 |
| `pepPrefixMatch` | `(v: Pep440Version, target: Pep440Version, prefix_len: u8): bool` | 调用 `pepPrefixMatch` 并返回 `bool`。 |
| `pepMatchSingle` | `(v: Pep440Version, raw: string): bool` | 调用 `pepMatchSingle` 并返回 `bool`。 |
| `matchesSpecifier` | `(ver: string, spec: string): bool` | 调用 `matchesSpecifier` 并返回 `bool`。 |

## 最小调用示例

```cp
import { parsePep440 } from "pep440_rs";

pub fn main() {
    let result = parsePep440("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
