[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/regex-escape

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:regex-escape` ([https://github.com/rust-lang/regex](https://github.com/rust-lang/regex))

Zero-heap regular expression literal escaper, character-class escaper, unescaper, and metacharacter detector in pure CP.

## Features

- `isMetaChar(c)` & `needsEscape(input)`: fast-path metacharacter detection
- `escapedLen(input)`: pre-computes exact escaped byte length
- `escapeRegex(input, out)`: escapes all regex metacharacters (`\.+*?()|[]{}^$-`) plus `/`
- `escapeCharClass(input, out)`: escapes `\`, `]`, `-`, `^` inside `[...]`
- `unescapeRegexLiteral(input, out)`: reverses `\x` literal escapes

## Usage

```cp
import { escapeRegex } from "@cap/regex-escape";

let buf: [64]u8 = undefined;
const n = escapeRegex("price: $9.99 (USD)", buf[0..64]);
// buf[0..n] => "price: \\$9\\.99 \\(USD\\)"
```
