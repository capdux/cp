[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# env_filter

Zero-allocation RUST_LOG-style hierarchical log filter parser and longest-prefix module matcher (Rust env_filter parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:env_filter` ([https://github.com/rust-cli/env_logger](https://github.com/rust-cli/env_logger))

| 元信息 | 值 |
| --- | --- |
| 包名 | `env_filter` |
| 版本 | `1.0.0` |
| 许可证 | `MIT OR Apache-2.0` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add env_filter
```

```cp
import {
    LogLevel,
    FilterDirective,
    EnvFilter,
    efltLevelRank,
    parseLogLevel,
    efltMatchesTarget,
} from "env_filter";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `LogLevel` | `Off, Error, Warn, Info, Debug, Trace` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `FilterDirective` | `class` | 字段: `target: string, level: LogLevel` |
| `EnvFilter` | `class` | 字段: `directives: [32]FilterDirective, count: usize` · 方法: `empty, sortDesc, addDirective, parse, directiveCount, getDirective, +2 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `efltLevelRank` | `(lvl: LogLevel): u8` | 调用 `efltLevelRank` 并返回 `u8`。 |
| `parseLogLevel` | `(s: string): LogLevel?` | 通过 `parseLogLevel` 解析输入数据，返回 `LogLevel?`。 |
| `efltMatchesTarget` | `(target: string, directive_target: string): bool` | 调用 `efltMatchesTarget` 并返回 `bool`。 |

## 最小调用示例

```cp
import { parseLogLevel } from "env_filter";

pub fn main() {
    let result = parseLogLevel("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
