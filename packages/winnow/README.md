[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# winnow

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:winnow` ([https://github.com/winnow-rs/winnow](https://github.com/winnow-rs/winnow))（原库名 `winnow` → CP 包名 `winnow`）

Zero-allocation byte-level zero-copy parser combinators for CP (`winnow` / `nom` parity).

## Features

- **Zero heap allocation**: `Cursor` slices directly over `string` input with strict rollback-on-failure semantics.
- **Core combinators**: `tag`, `tagNoCase`, `charLit`, `take`, `takeUntil`, `takeTillByte`, `space0`, `multispace0`, `digit1`, `alpha1`, `alphanumeric1`, `hexDigit1`, `parseU64`, `parseI64`, and `delimited`.

## Usage

```cp
import { Cursor, alpha1, multispace0, charLit, parseI64 } from "winnow";

let cur = Cursor.new("  timeout = -42");
* = multispace0(&cur);
const key = alpha1(&cur);
* = multispace0(&cur);
* = charLit(&cur, '=' as u8);
* = multispace0(&cur);
const val = parseI64(&cur); // val.value == -42
```
