# badge-maker — AI Agent Quick Ref

Zero-heap Shields.io-style SVG status badge generator (flat, flat-square, plastic) with XML escaping and named colors (`v1.0.0`, `MIT`)

## Import

```cp
import {
    BadgeStyle,
    BadgeOptions,
    BadgeWriter,
    defaultBadge,
    resolveBadgeColor,
    estimateTextWidth,
    renderBadge,
    makeBadge,
} from "badge-maker";
```

## Core Types & Constants

```cp
enum BadgeStyle { Flat, FlatSquare, Plastic }
class BadgeOptions { label: string, message: string, labelColor: string, color: string, /* +1 fields */ }
class BadgeWriter { out: []u8, pos: usize } // methods: putStr(...) +2
```

## API Signatures

```cp
fn defaultBadge(label: string, message: string, color: string): BadgeOptions;
fn resolveBadgeColor(name: string): string;
fn estimateTextWidth(s: string): usize;
fn renderBadge(opts: BadgeOptions, out: []u8): string;
fn makeBadge(label: string, message: string, color: string, out: []u8): string;
```

## Minimal Usage

```cp
import { defaultBadge } from "badge-maker";

pub fn main() {
    let result = defaultBadge("example", "example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"badge-maker"`; install via `cap add badge-maker`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
