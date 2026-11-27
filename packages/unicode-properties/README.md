[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# unicode-properties

Zero-allocation Unicode character properties, UAX #31 identifier validation (XID_Start/XID_Continue), White_Space, Default_Ignorable, and UTS #51 Emoji classification in pure CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:unicode-properties` ([https://github.com/unicode-rs/unicode-properties](https://github.com/unicode-rs/unicode-properties))

| Metadata | Value |
| --- | --- |
| Package | `unicode-properties` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add unicode-properties
```

```cp
import {
    decodeUtf8Strict,
    isXidStart,
    isXidContinue,
    isUnicodeWhitespace,
    isEmojiPresentation,
    isEmoji,
    isDefaultIgnorable,
    isValidIdentifier,
} from "unicode-properties";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `decodeUtf8Strict` | `(s: string, pos: usize, next_pos: &mut usize): u32` | Parses input via `decodeUtf8Strict` and returns `u32`. |
| `isXidStart` | `(cp: u32): bool` | Checks predicate `isXidStart` and returns `bool`. |
| `isXidContinue` | `(cp: u32): bool` | Checks predicate `isXidContinue` and returns `bool`. |
| `isUnicodeWhitespace` | `(cp: u32): bool` | Checks predicate `isUnicodeWhitespace` and returns `bool`. |
| `isEmojiPresentation` | `(cp: u32): bool` | Checks predicate `isEmojiPresentation` and returns `bool`. |
| `isEmoji` | `(cp: u32): bool` | Checks predicate `isEmoji` and returns `bool`. |
| `isDefaultIgnorable` | `(cp: u32): bool` | Checks predicate `isDefaultIgnorable` and returns `bool`. |
| `isValidIdentifier` | `(s: string): bool` | Checks predicate `isValidIdentifier` and returns `bool`. |

## Minimal Example

```cp
import { decodeUtf8Strict } from "unicode-properties";

pub fn main() {
    let result = decodeUtf8Strict("example", 0, /* next_pos: &mut usize */);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
