[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# execa

Zero-heap subprocess command builder, POSIX/Windows argument escaper, preferLocal PATH resolver, and ExecaResult inspector (npm execa equivalent, reuses npm-run-path)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:execa` ([https://github.com/sindresorhus/execa](https://github.com/sindresorhus/execa))

| 元信息 | 值 |
| --- | --- |
| 包名 | `execa` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add execa
```

```cp
import {
    ExecaOptions,
    ExecaResult,
    exaPush,
    exaPushByte,
    exaNeedsQuote,
    escapePosixArg,
    formatCommand,
    stripFinalNewline,
} from "execa";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ExecaOptions` | `class` | 字段: `cwd: string, path_env: string, prefer_local: bool, reject: bool, strip_final_newline: bool, timeout_ms: u64, +1 项` · 方法: `defaults` |
| `ExecaResult` | `class` | 字段: `exit_code: i32, signal_code: i32, timed_out: bool, is_canceled: bool, failed: bool, stdout: string, +1 项` · 方法: `success, failure, timeout, isOk` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `exaPush` | `(out: []u8, pos: usize, s: string): usize` | 调用 `exaPush` 并返回 `usize`。 |
| `exaPushByte` | `(out: []u8, pos: usize, b: u8): usize` | 调用 `exaPushByte` 并返回 `usize`。 |
| `exaNeedsQuote` | `(arg: string): bool` | 调用 `exaNeedsQuote` 并返回 `bool`。 |
| `escapePosixArg` | `(arg: string, out: []u8): usize` | 调用 `escapePosixArg` 并返回 `usize`。 |
| `formatCommand` | `(file: string, args: []string, out: []u8): usize` | 通过 `formatCommand` 编码或格式化输出，返回 `usize`。 |
| `stripFinalNewline` | `(s: string): string` | 调用 `stripFinalNewline` 并返回 `string`。 |
| `resolveExecPath` | `(opts: ExecaOptions, out: []u8): usize` | 调用 `resolveExecPath` 并返回 `usize`。 |

## 最小调用示例

```cp
import { formatCommand } from "execa";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatCommand("example", /* args: []string */, out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
