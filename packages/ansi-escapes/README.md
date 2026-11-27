[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# ansi-escapes

Zero-allocation ANSI/VT100 terminal cursor control, line/screen erasing, OSC-8 hyperlinks, and window title escape sequence engine (ansi-escapes parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:ansi-escapes` ([https://github.com/sindresorhus/ansi-escapes](https://github.com/sindresorhus/ansi-escapes))

| Metadata | Value |
| --- | --- |
| Package | `ansi-escapes` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add ansi-escapes
```

```cp
import {
    pushByte,
    pushStr,
    pushUsize,
    cursorTo,
    cursorToXY,
    cursorMove,
    cursorUp,
    cursorDown,
} from "ansi-escapes";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `pushByte` | `(out: []u8, pos: usize, b: u8): usize` | Mutates or appends state via `pushByte`. |
| `pushStr` | `(out: []u8, pos: usize, s: string): usize` | Mutates or appends state via `pushStr`. |
| `pushUsize` | `(out: []u8, pos: usize, val: usize): usize` | Mutates or appends state via `pushUsize`. |
| `cursorTo` | `(x: usize, out: []u8): string` | Invokes `cursorTo` and returns `string`. |
| `cursorToXY` | `(x: usize, y: usize, out: []u8): string` | Invokes `cursorToXY` and returns `string`. |
| `cursorMove` | `(dx: i32, dy: i32, out: []u8): string` | Invokes `cursorMove` and returns `string`. |
| `cursorUp` | `(n: usize, out: []u8): string` | Invokes `cursorUp` and returns `string`. |
| `cursorDown` | `(n: usize, out: []u8): string` | Invokes `cursorDown` and returns `string`. |
| `cursorForward` | `(n: usize, out: []u8): string` | Invokes `cursorForward` and returns `string`. |
| `cursorBackward` | `(n: usize, out: []u8): string` | Invokes `cursorBackward` and returns `string`. |
| `cursorLeft` | `(): string` | Invokes `cursorLeft` and returns `string`. |
| `cursorSave` | `(): string` | Invokes `cursorSave` and returns `string`. |
| `cursorRestore` | `(): string` | Invokes `cursorRestore` and returns `string`. |
| `cursorHide` | `(): string` | Invokes `cursorHide` and returns `string`. |
| `cursorShow` | `(): string` | Invokes `cursorShow` and returns `string`. |
| `eraseLine` | `(): string` | Invokes `eraseLine` and returns `string`. |
| `eraseEndLine` | `(): string` | Invokes `eraseEndLine` and returns `string`. |
| `eraseStartLine` | `(): string` | Invokes `eraseStartLine` and returns `string`. |
| `eraseDown` | `(): string` | Invokes `eraseDown` and returns `string`. |
| `eraseUp` | `(): string` | Invokes `eraseUp` and returns `string`. |
| `eraseScreen` | `(): string` | Invokes `eraseScreen` and returns `string`. |
| `clearTerminal` | `(): string` | Invokes `clearTerminal` and returns `string`. |
| `scrollUp` | `(): string` | Invokes `scrollUp` and returns `string`. |
| `scrollDown` | `(): string` | Invokes `scrollDown` and returns `string`. |
| `eraseLines` | `(count: usize, out: []u8): string` | Invokes `eraseLines` and returns `string`. |
| `hyperlink` | `(text: string, url: string, out: []u8): string` | Invokes `hyperlink` and returns `string`. |
| `setWindowTitle` | `(title: string, out: []u8): string` | Mutates or appends state via `setWindowTitle`. |
| `beep` | `(): string` | Invokes `beep` and returns `string`. |

## Minimal Example

```cp
import { pushStr } from "ansi-escapes";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = pushStr(out_buf[0..256], 0, "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
