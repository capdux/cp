[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# `pad`

> **Source Attribution**: `npm:pad` ([adaltas/node-pad](https://github.com/adaltas/node-pad)), `crates.io:pad` ([ogham/rust-pad](https://github.com/ogham/rust-pad)), & `npm:left-pad` — License: MIT

Zero-allocation UTF-8 codepoint-aware string padding (`pad`, `padStart`, `padEnd`, `padLeft`, `padRight`, `padCenter`, `padCenterWith`), truncation (`truncateChars`), and string repetition (`repeatStr`) in pure CP.

## Zero-Cost Migration (TS / Rust → CP)

Keep the exact upstream package name (`pad`) — no renaming required.

```typescript
// TypeScript / Node.js (npm: pad)
import pad from "pad";
const left = pad(8, "hi", "0");      // "000000hi"
const right = pad("hi", 8, ".");     // "hi......"
```

```rust
// Rust (crates.io: pad)
use pad::{PadStr, Alignment};
let s = "hi".pad_to_width_with_alignment(8, Alignment::Middle);
```

```cp
// CP (zero-heap, UTF-8 codepoint-aware)
import { padStart, padEnd, padCenter } from "pad";

let buf: [64]u8 = undefined;
let n1 = padStart("hi", 8, '0', buf[0..64]);   // "000000hi"
let n2 = padEnd("hi", 8, '.', buf[0..64]);     // "hi......"
let n3 = padCenter("hi", 8, ' ', buf[0..64]);  // "   hi   "
```

## Installation

Add `pad` to your `cap.json`:

```json
{
  "dependencies": {
    "pad": "1.0.0"
  }
}
```

## API Overview

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| `PadOptions` | `class { pad_char: u8, pad_str: string, truncate: bool, left: bool, fn defaults(): PadOptions }` | Configurable padding options (multi-char pattern, truncation, alignment) |
| `pad` | `(s: string, target_chars: usize, opts: PadOptions, out: []u8): usize` | Full-featured pad supporting left/right alignment, custom `pad_str`, and truncation |
| `padCenterWith` | `(s: string, target_chars: usize, opts: PadOptions, out: []u8): usize` | Center-pad with `PadOptions` (`pad_str` & `truncate`) |
| `padStart` / `padLeft` | `(s: string, target_chars: usize, pad_char: u8, out: []u8): usize` | Left-pad with single ASCII byte to `target_chars` codepoints |
| `padEnd` / `padRight` | `(s: string, target_chars: usize, pad_char: u8, out: []u8): usize` | Right-pad with single ASCII byte to `target_chars` codepoints |
| `padCenter` | `(s: string, target_chars: usize, pad_char: u8, out: []u8): usize` | Center-pad with single ASCII byte |
| `repeatStr` | `(s: string, count: usize, out: []u8): usize` | Repeat string `count` times into `out` |
| `countChars` | `(s: string): usize` | Count UTF-8 Unicode codepoints in `s` |
| `truncateChars` | `(s: string, max_chars: usize): string` | Zero-copy UTF-8 codepoint-safe prefix slice |
| `calcPadBytes` | `(pad_chars: usize, pad_char: u8, pad_str: string): usize` | Compute byte length required for padding pattern |
| `writePadPattern` | `(dst: []u8, pos: usize, pad_chars: usize, pad_char: u8, pad_str: string): usize` | Write repeated padding pattern at offset `pos` |
| `copyBytes` / `fillByte` | `(dst: []u8, pos: usize, src: string)` / `(dst: []u8, pos: usize, count: usize, ch: u8)` | Low-level buffer helpers |

## Practical Examples

### 1. Basic Padding & Repetition

```cp
import { padLeft, padRight, padCenter, repeatStr, countChars } from "pad";

let buf: [64]u8 = undefined;

let n1 = padLeft("42", 5, '0', buf[0..64]);     // "00042"
let n2 = padRight("OK", 6, '.', buf[0..64]);    // "OK...."
let n3 = padCenter("CP", 6, '-', buf[0..64]);   // "--CP--"
let n4 = repeatStr("ab", 3, buf[0..64]);        // "ababab"
let chars = countChars("你好CP");                // 4 (codepoints, not 8 bytes)
```

### 2. Multi-Character Pattern & Truncation with `PadOptions`

```cp
import { PadOptions, pad, padCenterWith, truncateChars } from "pad";

let buf: [64]u8 = undefined;

let opts = PadOptions.defaults();
opts.pad_str = ".-";
opts.left = true;
opts.truncate = true;

let n1 = pad("hello world", 5, opts, buf[0..64]);       // "hello" (truncated)
let n2 = pad("hi", 6, opts, buf[0..64]);                // ".-.-hi"
let n3 = padCenterWith("CP", 6, opts, buf[0..64]);      // ".-CP.-"

let short = truncateChars("你好世界", 2);                // "你好" (zero-copy)
```

## Notes & Edge Cases

- **UTF-8 codepoint aware**: `countChars`, `truncateChars`, and all `pad*` functions measure width in UTF-8 codepoints so multi-byte characters (such as CJK or emoji) are never split mid-sequence.
- **Zero heap allocations**: All functions write into caller-supplied `[]u8` buffers and return the written byte count.
- **Truncation**: When `opts.truncate` is `false` (default), strings longer than `target_chars` are copied verbatim without truncation.
