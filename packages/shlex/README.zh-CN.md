[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# shlex

Zero-allocation POSIX shell quoting, command-line lexical splitting & injection guard (shell-quote / Rust shlex parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:shell-quote` ([https://github.com/ljharb/shell-quote](https://github.com/ljharb/shell-quote)) / `npm:shell-quote` ([https://github.com/inspect-js/node-shell-quote](https://github.com/inspect-js/node-shell-quote)) / `crates.io:shlex` ([https://github.com/comex/rust-shlex](https://github.com/comex/rust-shlex))（原库名 `shell-quote` → CP 包名 `shlex`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `shlex` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add shlex
```

```cp
import {
    ShlexError,
    SplitResult,
    isSafeUnquotedChar,
    needsQuote,
    hasShellMetachars,
    appendByte,
    quoteArg,
    joinArgs,
} from "shlex";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `ShlexError` | `None, UnclosedSingleQuote, UnclosedDoubleQuote, TrailingBackslash, TooManyTokens, BufferOverflow` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `SplitResult` | `class` | 字段: `pool: [MAX_POOL_BYTES]u8, starts: [MAX_TOKENS]usize, lens: [MAX_TOKENS]usize, count: usize, ok: bool, err: ShlexError` · 方法: `empty, tokenAt` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `isSafeUnquotedChar` | `(c: u8): bool` | 执行 `isSafeUnquotedChar` 条件判定，返回 `bool`。 |
| `needsQuote` | `(arg: string): bool` | 调用 `needsQuote` 并返回 `bool`。 |
| `hasShellMetachars` | `(s: string): bool` | 执行 `hasShellMetachars` 条件判定，返回 `bool`。 |
| `appendByte` | `(out: []u8, pos: usize, c: u8): usize` | 调用 `appendByte` 并返回 `usize`。 |
| `quoteArg` | `(arg: string, out: []u8): string` | 调用 `quoteArg` 并返回 `string`。 |
| `joinArgs` | `(args: []string, out: []u8): string` | 调用 `joinArgs` 并返回 `string`。 |
| `splitShell` | `(cmd: string): SplitResult` | 调用 `splitShell` 并返回 `SplitResult`。 |

## 最小调用示例

```cp
import { needsQuote } from "shlex";

pub fn main() {
    let result = needsQuote("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
