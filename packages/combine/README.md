[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/combine

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:combine` ([https://github.com/Marwes/combine](https://github.com/Marwes/combine))

Zero-copy streaming parser combinators with line/column cursor tracking and backtracking for CP — clean-room parity with `combine` v4 (354M+ downloads on crates.io, MIT).

纯 CP 零拷贝流式解析器组合子库，提供带行列号追踪与 `save`/`restore` 回溯的 `ParseCursor`，以及 `tag`、`charByte`、`takeWhile`、`takeWhile1`、`takeUntil`、`skipSpaces`、`parseU64`、`parseI64`、`parseQuotedString`、`delimited`、`separatedList` 等常用组合子原语。

## Install / 安装

```bash
cap add @cap/combine
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `ParseCursor` | `new`, `remaining`, `isEof`, `peek`, `advance`, `save`, `restore` | Zero-copy backtracking cursor with line/col tracking |
| `CharPred` | `Alpha`, `Digit`, `AlphaNum`, `Space`, `HexDigit`, `Ident`, `NonSpace` | Character class predicates |
| `tag` / `charByte` | `(cursor: *ParseCursor, ...): bool` | Literal & byte matchers |
| `takeWhile` / `takeWhile1` / `takeUntil` | `(cursor: *ParseCursor, ...)` | Zero-copy slice consumers |
| `skipSpaces` / `parseU64` / `parseI64` | `(cursor: *ParseCursor)` | Whitespace & integer parsers |
| `parseQuotedString` / `delimited` / `separatedList` | `(cursor: *ParseCursor, ...)` | Quoted string, bracketed & separated list combinators |

## Usage / 使用示例

```cp
import { ParseCursor, tag, skipSpaces, parseU64 } from "@cap/combine";

let c = ParseCursor.new("port = 8080");
const ok = tag(*c, "port");
* = skipSpaces(*c);
* = tag(*c, "=");
* = skipSpaces(*c);
const port = parseU64(*c); // 8080
```
