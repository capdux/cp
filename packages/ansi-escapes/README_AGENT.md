# ansi-escapes — AI Agent Quick Ref

Zero-allocation ANSI/VT100 terminal cursor control, line/screen erasing, OSC-8 hyperlinks, and window title escape sequence engine (ansi-escapes parity) for CP (`v1.0.0`, `MIT`)

## Import

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

## API Signatures

```cp
fn pushByte(out: []u8, pos: usize, b: u8): usize;
fn pushStr(out: []u8, pos: usize, s: string): usize;
fn pushUsize(out: []u8, pos: usize, val: usize): usize;
fn cursorTo(x: usize, out: []u8): string;
fn cursorToXY(x: usize, y: usize, out: []u8): string;
fn cursorMove(dx: i32, dy: i32, out: []u8): string;
fn cursorUp(n: usize, out: []u8): string;
fn cursorDown(n: usize, out: []u8): string;
fn cursorForward(n: usize, out: []u8): string;
fn cursorBackward(n: usize, out: []u8): string;
fn cursorLeft(): string;
fn cursorSave(): string;
fn cursorRestore(): string;
fn cursorHide(): string;
fn cursorShow(): string;
fn eraseLine(): string;
fn eraseEndLine(): string;
fn eraseStartLine(): string;
fn eraseDown(): string;
fn eraseUp(): string;
fn eraseScreen(): string;
fn clearTerminal(): string;
fn scrollUp(): string;
fn scrollDown(): string;
fn eraseLines(count: usize, out: []u8): string;
fn hyperlink(text: string, url: string, out: []u8): string;
fn setWindowTitle(title: string, out: []u8): string;
fn beep(): string;
```

## Minimal Usage

```cp
import { pushStr } from "ansi-escapes";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = pushStr(out_buf[0..256], 0, "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"ansi-escapes"`; install via `cap add ansi-escapes`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
