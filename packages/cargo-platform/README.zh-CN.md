[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# cargo-platform

Zero-allocation LLVM/Rust target triple parser and Cargo cfg(...) expression evaluator for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:cargo-platform` ([https://github.com/rust-lang/cargo](https://github.com/rust-lang/cargo))

| 元信息 | 值 |
| --- | --- |
| 包名 | `cargo-platform` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add cargo-platform
```

```cp
import {
    TargetTriple,
    TargetEnv,
    CfgEvalResult,
    CfgParser,
    invalidTriple,
    parseTargetTriple,
    isIdentStart,
    isIdentCont,
} from "cargo-platform";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `TargetTriple` | `class` | 字段: `arch_start: usize, arch_len: usize, vendor_start: usize, vendor_len: usize, os_start: usize, os_len: usize, +4 项` |
| `TargetEnv` | `class` | 字段: `arch: string, os: string, family: string, env: string, pointer_width: u32, is_big_endian: bool` |
| `CfgEvalResult` | `class` | 字段: `matched: bool, ok: bool` |
| `CfgParser` | `class` | 字段: `pos: usize, ok: bool, depth: usize` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `invalidTriple` | `(): TargetTriple` | 调用 `invalidTriple` 并返回 `TargetTriple`。 |
| `parseTargetTriple` | `(triple: string): TargetTriple` | 通过 `parseTargetTriple` 解析输入数据，返回 `TargetTriple`。 |
| `isIdentStart` | `(c: u8): bool` | 执行 `isIdentStart` 条件判定，返回 `bool`。 |
| `isIdentCont` | `(c: u8): bool` | 执行 `isIdentCont` 条件判定，返回 `bool`。 |
| `skipWs` | `(p: &mut CfgParser, src: string)` | 就地执行 `skipWs` 操作。 |
| `u32MatchesDecimal` | `(n: u32, s: string): bool` | 调用 `u32MatchesDecimal` 并返回 `bool`。 |
| `evalKeyValue` | `(key: string, val: string, target: TargetEnv): bool` | 调用 `evalKeyValue` 并返回 `bool`。 |
| `evalFlag` | `(flag: string, target: TargetEnv): bool` | 调用 `evalFlag` 并返回 `bool`。 |
| `parsePredicate` | `(p: &mut CfgParser, src: string, target: TargetEnv): bool` | 通过 `parsePredicate` 解析输入数据，返回 `bool`。 |
| `evalCfgExpr` | `(expr: string, target: TargetEnv): CfgEvalResult` | 调用 `evalCfgExpr` 并返回 `CfgEvalResult`。 |

## 最小调用示例

```cp
import { parseTargetTriple } from "cargo-platform";

pub fn main() {
    let result = parseTargetTriple("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
