[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# ascii

Zero-heap ASCII validation, typed `AsciiChar` / `AsciiStr` wrappers, byte classification, case conversion, trimming, and case-insensitive comparison in pure CP.

- **Upstream Alignment**: `crates.io` `ascii` `v1.1.0` (MIT, tomprogrammer/rust-ascii)
- **Package Version**: `1.1.0`
- **Memory Model**: Zero heap allocation — borrowed `AsciiStr` slices, in-place `[]u8` mutators, and stack-buffer writers.

## Installation & Import

```bash
cap add ascii
```

```cp
import {
    AsciiChar,
    AsciiStr,
    isAsciiAlpha,
    isAsciiDigit,
    isAsciiAlphanumeric,
    isAsciiHexDigit,
    isAsciiOctDigit,
    isAsciiWhitespace,
    isAsciiPrintable,
    isAsciiControl,
    isAsciiLowercase,
    isAsciiUppercase,
    isAsciiGraph,
    isAsciiPunctuation,
    toAsciiLowerByte,
    toAsciiUpperByte,
    eqIgnoreAsciiCase,
    toAsciiLower,
    toAsciiUpper,
    toAsciiLowercase,
    toAsciiUppercase,
    makeAsciiLowercase,
    makeAsciiUppercase,
    trimAscii,
} from "ascii";
```

## Core API

| Symbol | Signature | Description |
| --- | --- | --- |
| Byte predicates | `isAsciiAlpha`, `isAsciiDigit`, `isAsciiAlphanumeric`, `isAsciiHexDigit`, `isAsciiOctDigit`, `isAsciiWhitespace`, `isAsciiPrintable`, `isAsciiControl`, `isAsciiLowercase`, `isAsciiUppercase`, `isAsciiGraph`, `isAsciiPunctuation` (`(b: u8): bool`) | Complete ASCII byte classification predicates. |
| Byte case helpers | `toAsciiLowerByte(b: u8): u8`, `toAsciiUpperByte(b: u8): u8` | Converts a single ASCII letter byte to lowercase / uppercase; non-letters pass through unchanged. |
| `AsciiChar` | `class AsciiChar { byte: u8, ... }` | Validated 7-bit ASCII byte (`fromByte(b): AsciiChar?`, `asByte()`, `isAlphabetic()`, `isAlphanumeric()`, `isControl()`, `isDigit()`, `isGraph()`, `isHexDigit()`, `isLowercase()`, `isPrintable()`, `isPunctuation()`, `isUppercase()`, `isWhitespace()`, `toAsciiUppercase()`, `toAsciiLowercase()`, `eqIgnoreAsciiCase(other)`). |
| `AsciiStr` | `class AsciiStr { bytes: string, ... }` | Validated ASCII string view (`fromStr(s): AsciiStr?`, `len()`, `isEmpty()`, `asStr()`, `get(idx): AsciiChar?`, `eqIgnoreAsciiCase(other)`, `trim()`, `trimStart()`, `trimEnd()`, `toAsciiLowercase(out): usize`, `toAsciiUppercase(out): usize`). |
| `eqIgnoreAsciiCase` | `fn eqIgnoreAsciiCase(a: string, b: string): bool` | Case-insensitive ASCII string equality. |
| `toAsciiLower` / `toAsciiLowercase` | `fn toAsciiLower(s: string, out: []u8): usize` | Writes ASCII-lowercased copy of `s` into `out`; returns bytes written. |
| `toAsciiUpper` / `toAsciiUppercase` | `fn toAsciiUpper(s: string, out: []u8): usize` | Writes ASCII-uppercased copy of `s` into `out`; returns bytes written. |
| `makeAsciiLowercase` / `makeAsciiUppercase` | `fn makeAsciiLowercase(buf: []u8)` / `fn makeAsciiUppercase(buf: []u8)` | Mutates ASCII letters in `buf` in place with zero extra memory. |
| `trimAscii` | `fn trimAscii(s: string): string` | Trims leading and trailing ASCII whitespace, returning a zero-copy sub-slice. |

## Examples

### 1. Validated `AsciiStr` and `AsciiChar`

```cp
import { AsciiStr, AsciiChar } from "ascii";

fn validateDemo(): void {
    if (let s = AsciiStr.fromStr("  Hello-CP  ")) {
        const trimmed = s.trim(); // "Hello-CP"
        if (let ch = trimmed.get(0)) {
            const lower = ch.toAsciiLowercase().asByte(); // 'h'
        }
    }
    const invalid = AsciiChar.fromByte(0xFF); // null (non-ASCII)
}
```

### 2. In-Place & Buffer Case Conversion

```cp
import { toAsciiLower, makeAsciiUppercase, eqIgnoreAsciiCase } from "ascii";

fn caseDemo(): void {
    let mut buf: [16]u8 = [0; 16];
    const n = toAsciiLower("Content-Type", buf[0..]); // "content-type"
    makeAsciiUppercase(buf[0..n]);                     // "CONTENT-TYPE"
    const same = eqIgnoreAsciiCase("HTTP", "http");    // true
}
```

## Differences from Upstream & Highlights

- **Zero-Copy Borrowed Views**: `AsciiStr` wraps a borrowed `string` slice directly; `trim()`, `trimStart()`, and `trimEnd()` return sub-slices with zero allocations.
- **Explicit Output Buffers**: Instead of returning heap-allocated `AsciiString`, `toAsciiLowercase`/`toAsciiUppercase` write into caller-provided `out: []u8`, alongside in-place `makeAsciiLowercase`/`makeAsciiUppercase`.
- **Standard Library Synergy**: Builds cleanly on `cap:ascii` primitives while exposing the ergonomic Rust `ascii` type model.
