# colorette — AI Agent Quick Ref

Zero-heap pure CP terminal ANSI color formatter with nested close-code recovery (clean-room MIT implementation of npm colorette v2) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    ColoretteOptions,
    Colorette,
    clrFormatPair,
    isColorSupported,
    createColors,
    createColorsWithOptions,
} from "colorette";
```

## Core Types & Constants

```cp
class ColoretteOptions { use_color: bool } // methods: enabled(...) +2
class Colorette { options: ColoretteOptions } // methods: new(...) +45
```

## API Signatures

```cp
fn clrFormatPair(use_color: bool, pair: StylePair, replace_with: string, text: string, out: []u8): usize;
fn isColorSupported(no_color: string, force_color: string, term: string, is_tty: bool): bool;
fn createColors(use_color: bool): Colorette;
fn createColorsWithOptions(opts: ColoretteOptions): Colorette;
```

## Minimal Usage

```cp
import { isColorSupported } from "colorette";

pub fn main() {
    let result = isColorSupported("example", "example", "example", false);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"colorette"`; install via `cap add colorette`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
