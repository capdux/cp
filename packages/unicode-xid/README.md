[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `@cap/unicode-xid`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:unicode-xid` ([https://github.com/unicode-rs/unicode-xid](https://github.com/unicode-rs/unicode-xid))

Zero-allocation **UAX #31** Unicode Identifier (`XID_Start` / `XID_Continue`) classifier and lexer scanner for CP (clean-room MIT implementation inspired by Rust's `unicode-xid`).

## Features

- **Zero Heap Allocation**: Static sorted range tables with O(log N) binary search and single-branch ASCII fast paths.
- **Unicode 15.1 Script Coverage**: ASCII, Latin-1/Extended, Greek, Cyrillic, Armenian, Hebrew, Arabic, Devanagari, Thai, Hiragana, Katakana, Hangul Syllables/Jamo, CJK Unified Ideographs & Extensions, combining marks, Unicode digits, and connector punctuation.
- **Strict UTF-8 Validation**: Rejects truncated, overlong, and surrogate UTF-8 sequences when validating or scanning identifiers.
- **Rust/CP Identifier Rules**: `isIdentStart` supports `'_'` or `XID_Start`; `isValidIdent` and `scanIdentLen` follow `('_' | XID_Start) XID_Continue*`.

## Usage

```typescript
import {
    isXidStart,
    isXidContinue,
    isIdentStart,
    isValidIdent,
    scanIdentLen,
    isAsciiIdent,
} from "@cap/unicode-xid";

const a: bool = isXidStart(0x03B1);           // 'α' -> true
const b: bool = isXidContinue(0x0301);        // combining acute -> true
const c: bool = isIdentStart(0x5F);           // '_' -> true
const d: bool = isValidIdent("变量_42");       // true
const e: bool = isAsciiIdent("hello_world");  // true
const n: usize = scanIdentLen("let αβ1 = 2", 4); // 5 bytes ("αβ1")
```

## API

- `isXidStart(cp: u32): bool`
- `isXidContinue(cp: u32): bool`
- `isIdentStart(cp: u32): bool`
- `isValidIdent(s: string): bool`
- `scanIdentLen(s: string, start: usize): usize`
- `isAsciiIdent(s: string): bool`

## License

MIT
