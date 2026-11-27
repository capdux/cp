# wcag-contrast — AI Agent Quick Ref

Zero-allocation W3C WCAG 2.1/2.2 color contrast ratio calculator, AA/AAA compliance grader, and accessible foreground color recommender (wcag-contrast / Rust contrast parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    WcagLevel,
    Rgb,
    hexNibble,
    nibbleToHexLower,
    parseHex,
    formatHex,
    linearizeChannel,
    relativeLuminance,
} from "wcag-contrast";
```

## Core Types & Constants

```cp
enum WcagLevel { Fail, AALarge, AA, AAA }
class Rgb { r: u8, g: u8, b: u8 }
```

## API Signatures

```cp
fn hexNibble(c: u8): i8;
fn nibbleToHexLower(n: u8): u8;
fn parseHex(hex: string): Rgb?;
fn formatHex(c: Rgb, out: []u8): string;
fn linearizeChannel(channel: u8): f64;
fn relativeLuminance(c: Rgb): f64;
fn contrastRatio(fg: Rgb, bg: Rgb): f64;
fn contrastRatioHex(fg_hex: string, bg_hex: string): f64?;
fn meetsAA(ratio: f64, large_text: bool): bool;
fn meetsAAA(ratio: f64, large_text: bool): bool;
fn gradeContrast(ratio: f64): WcagLevel;
fn bestTextColor(bg: Rgb): Rgb;
fn suggestAccessibleHex(bg_hex: string): string;
```

## Minimal Usage

```cp
import { parseHex } from "wcag-contrast";

pub fn main() {
    let result = parseHex("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"wcag-contrast"`; install via `cap add wcag-contrast`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
