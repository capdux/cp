# anstyle — AI Agent Quick Ref

Zero-heap ANSI terminal styling model (16-color, 256-color, truecolor RGB, SGR effects) in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Color,
    Effects,
    Style,
    writeByte,
    writeU8Dec,
    writeParam,
} from "anstyle";
```

## Core Types & Constants

```cp
class Color { kind: u8, r: u8, g: u8, b: u8 } // methods: none(...) +3
class Effects { bits: u16 } // methods: new(...) +1
class Style { fg: Color, bg: Color, effects: Effects } // methods: new(...) +14
```

## API Signatures

```cp
fn writeByte(out: []u8, pos: usize, b: u8): usize;
fn writeU8Dec(out: []u8, pos: usize, val: u8): usize;
fn writeParam(out: []u8, pos: usize, has_prev: bool, val: u8): usize;
```

## Minimal Usage

```cp
import { writeByte } from "anstyle";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = writeByte(out_buf[0..256], 0, 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"anstyle"`; install via `cap add anstyle`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
