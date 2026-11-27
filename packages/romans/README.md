[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# romans

Zero-allocation strict Roman numeral (`1..3999`) bidirectional parser, validator, and formatter in pure CP.

- **Upstream Alignment**: `npm` `romans` & `crates.io` `roman` (MIT)
- **Package Version**: `1.0.0`
- **Memory Model**: Zero heap allocation — caller supplies stack buffers (`[]u8`) for formatting.

## Installation & Import

```bash
cap add romans
```

```cp
import {
    MIN_ROMAN,
    MAX_ROMAN,
    MAX_ROMAN_LEN,
    toRoman,
    toRomanLower,
    romanize,
    romanizeChecked,
    parseRoman,
    deromanize,
    isValidRoman,
    romanLen,
    romanCharValue,
} from "romans";
```

## Core API

| Symbol | Signature | Description |
| --- | --- | --- |
| `MIN_ROMAN` | `const MIN_ROMAN: u16` | Minimum representable Roman numeral (`1`). |
| `MAX_ROMAN` | `const MAX_ROMAN: u16` | Maximum representable Roman numeral (`3999`). |
| `MAX_ROMAN_LEN` | `const MAX_ROMAN_LEN: usize` | Maximum byte length of any valid Roman numeral (`15`, e.g. `MMMDCCCLXXXVIII`). |
| `romanCharValue` | `fn romanCharValue(b: u8): i32` | Returns numeric value of an ASCII Roman digit (`I/V/X/L/C/D/M`, case-insensitive), or `-1` if invalid. |
| `romanLen` | `fn romanLen(num: u16): usize` | Exact formatted byte length for `num` (`0` if out of `1..3999`). |
| `toRoman` | `fn toRoman(num: u16, out: []u8): usize` | Formats `num` as uppercase Roman numerals into `out`; returns bytes written (`0` if out of range or buffer too small). |
| `toRomanLower` | `fn toRomanLower(num: u16, out: []u8): usize` | Formats `num` as lowercase Roman numerals into `out`; returns bytes written. |
| `romanize` | `fn romanize(num: i32, out: []u8): usize` | `i32` convenience wrapper for `toRoman` matching npm `romanize`; returns `0` if out of `1..3999`. |
| `romanizeChecked` | `fn romanizeChecked(num: i32, out: []u8): usize?` | Checked formatter returning `usize?` (`null` on invalid range or insufficient buffer). |
| `parseRoman` | `fn parseRoman(input: string): i32` | Strictly parses canonical Roman numerals (case-insensitive); returns `1..3999`, or `-1` on invalid input. |
| `deromanize` | `fn deromanize(input: string): i32?` | Optional-returning strict parser (`null` on invalid/non-canonical Roman numeral). |
| `isValidRoman` | `fn isValidRoman(input: string): bool` | Returns `true` iff `input` is a valid canonical Roman numeral in `1..3999`. |

## Examples

### 1. Format Numbers into Stack Buffer

```cp
import { toRoman, toRomanLower, MAX_ROMAN_LEN } from "romans";

fn formatExample(): void {
    let mut buf: [16]u8 = [0; 16];
    const n = toRoman(2026, buf[0..]);
    // buf[0..n] == "MMXXVI"

    const m = toRomanLower(42, buf[0..]);
    // buf[0..m] == "xlii"
}
```

### 2. Strict Parsing & Validation

```cp
import { deromanize, isValidRoman, parseRoman } from "romans";

fn parseExample(): void {
    const val = deromanize("MCMXCIV"); // Some(1994)
    const bad = deromanize("IIII");    // null (non-canonical)
    const ok = isValidRoman("xliv");   // true
    const raw = parseRoman("MMXXVI");  // 2026 (or -1 if invalid)
}
```

### 3. Checked Formatting with Pre-Sized Length

```cp
import { romanLen, romanizeChecked } from "romans";

fn checkedExample(): void {
    const needed = romanLen(3888); // 15 ("MMMDCCCLXXXVIII")
    let mut out: [16]u8 = [0; 16];
    if (let written = romanizeChecked(3888, out[0..])) {
        // out[0..written] holds the formatted numeral
    }
}
```

## Differences from Upstream & Highlights

- **Zero Heap Allocation**: Unlike JS `romans.romanize(n)` which allocates a new `string`, CP writes directly into a caller-owned mutable slice `out: []u8` (16 bytes is always sufficient since `MAX_ROMAN_LEN == 15`).
- **Strict Canonical Validation**: Rejects non-canonical forms such as `IIII`, `VV`, `IC`, and empty strings by verifying canonical round-trip equivalence.
- **No Exceptions**: Uses `usize?` / `i32?` optional return types or sentinel `0` / `-1` instead of throwing JS `Error`.
