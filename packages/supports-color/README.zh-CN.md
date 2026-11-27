[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# supports-color

Zero-allocation terminal color capability detection (supports-color v9 parity) powered by has-flag in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:supports-color` ([https://github.com/chalk/supports-color](https://github.com/chalk/supports-color))

| 元信息 | 值 |
| --- | --- |
| 包名 | `supports-color` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add supports-color
```

```cp
import {
    ColorSupport,
    TerminalEnv,
    scContainsIgnoreCase,
    translateLevel,
    scDetectCliOverride,
    scParseForceColor,
    detectColorLevel,
    detectColorSupport,
} from "supports-color";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ColorSupport` | `class` | 字段: `level: u8, has_basic: bool, has_256: bool, has_16m: bool` |
| `TerminalEnv` | `class` | 字段: `is_tty: bool, force_color: string, no_color: string, term: string, colorterm: string, term_program: string, +4 项` · 方法: `empty, tty` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `scContainsIgnoreCase` | `(haystack: string, needle: string): bool` | 调用 `scContainsIgnoreCase` 并返回 `bool`。 |
| `translateLevel` | `(level: u8): ColorSupport` | 调用 `translateLevel` 并返回 `ColorSupport`。 |
| `scDetectCliOverride` | `(argv: &[]string): i8` | 调用 `scDetectCliOverride` 并返回 `i8`。 |
| `scParseForceColor` | `(val: string): i8` | 调用 `scParseForceColor` 并返回 `i8`。 |
| `detectColorLevel` | `(env: TerminalEnv, argv: &[]string): u8` | 调用 `detectColorLevel` 并返回 `u8`。 |
| `detectColorSupport` | `(env: TerminalEnv, argv: &[]string): ColorSupport` | 调用 `detectColorSupport` 并返回 `ColorSupport`。 |

## 最小调用示例

```cp
import { scContainsIgnoreCase } from "supports-color";

pub fn main() {
    let result = scContainsIgnoreCase("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
