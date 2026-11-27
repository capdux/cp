# color-name — AI Agent Quick Ref

Zero-allocation 148 W3C CSS Color 4 named colors lookup and exact RGB/HSL/HSV/HEX color space conversion engine for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    RgbColor,
    ColorEntry,
    cnToLower,
    cnCmpIgnoreCase,
    colorAt,
    lookupColor,
    lookupColorHex,
    reverseLookupColor,
} from "color-name";
```

## Core Types & Constants

```cp
class RgbColor { r: u8, g: u8, b: u8, found: bool }
class ColorEntry { name: string, r: u8, g: u8, b: u8 }
```

## API Signatures

```cp
fn cnToLower(b: u8): u8;
fn cnCmpIgnoreCase(a: string, b: string): i8;
fn colorAt(index: usize): ColorEntry;
fn lookupColor(name: string): RgbColor;
fn lookupColorHex(name: string): i32;
fn reverseLookupColor(r: u8, g: u8, b: u8): string;
fn isNamedColor(name: string): bool;
```

## Minimal Usage

```cp
import { isNamedColor } from "color-name";

pub fn main() {
    let result = isNamedColor("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"color-name"`; install via `cap add color-name`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
