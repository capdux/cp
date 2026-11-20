# `cap:shell_parser` 标准库参考

## 1. 概述

`cap:shell_parser`（动态挂载路径 `cap.shell_parser`）是纯 CP 实现的工业级 POSIX Shell 词法分词、命令/管道/重定向 AST 解析与安全参数引用转义标准库，零外部依赖、全内存显式托管（`deinit(alloc)` 零泄漏）。

## 2. 核心类型

- **`ShellError`**：`error{ UnterminatedQuote, TrailingEscape, UnexpectedToken, OutOfMemory }`
- **`TokenKind`**：`Word`、`Pipe` (`|`)、`Or` (`||`)、`And` (`&&`)、`Semi` (`;`)、`Background` (`&`)、`RedirectOut` (`>`)、`RedirectAppend` (`>>`)、`RedirectErrOut` (`2>`)、`RedirectErrAppend` (`2>>`)、`RedirectIn` (`<`)、`RedirectErrToOut` (`2>&1`)、`RedirectBothOut` (`&>`)
- **`ShellToken`**：`{ kind: TokenKind, value: []const u8, quoted: bool, has_var: bool, start: usize, end: usize }`
- **`RedirectKind` / `Redirect`**：重定向类型与目标路径 `{ kind: RedirectKind, target: []const u8 }`
- **`Assignment`**：前置环境变量赋值 `{ name: []const u8, value: []const u8 }`
- **`SimpleCommand`**：单条命令 `{ assignments: []Assignment, argv: [][]const u8, redirects: []Redirect }`
- **`Pipeline`**：管道序列 `{ commands: []SimpleCommand, background: bool }`
- **`ListOp` / `ListItem` / `ShellScript`**：控制流链表（`Seq` / `And` / `Or`）与脚本根节点，均提供 `deinit(alloc)`

## 3. API 速查

| 函数 | 签名 | 说明 |
| --- | --- | --- |
| `tokenize` | `(alloc: Allocator, input: []const u8): ShellError![]ShellToken` | 词法分词，支持单/双引号、反斜杠转义/续行、`#` 注释与变量识别 |
| `deinitTokens` | `(alloc: Allocator, tokens: []ShellToken)` | 释放 `tokenize` 返回的 token 切片及内部字符串 |
| `parseScript` | `(alloc: Allocator, input: []const u8): ShellError!ShellScript` | 解析完整 Shell 脚本为 `ShellScript` AST |
| `needsQuoting` | `(arg: []const u8): bool` | 判断参数是否包含需要转义的字符或为空串 |
| `quoteArg` | `(alloc: Allocator, arg: []const u8): ![]u8` | 对单个参数进行防注入单引号转义（含 `'\''` 处理） |
| `joinCommand` | `(alloc: Allocator, argv: []const []const u8): ![]u8` | 安全转义并拼接整条命令行 |

## 4. 示例

```cp
import { parseScript, quoteArg } from "cap:shell_parser";
const std = @import("std");

let script = try parseScript(alloc, "FOO=bar cmd arg1 > out.txt 2>&1 | grep ok && echo done");
defer script.deinit(alloc);

const safe = try quoteArg(alloc, "it's $HOME");
defer alloc.free(safe); // "'it'\\''s $HOME'"
```
