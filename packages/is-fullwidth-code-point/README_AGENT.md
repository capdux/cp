# is-fullwidth-code-point — AI Agent Quick Ref

Zero-allocation UAX #11 Unicode fullwidth code point checker and UTF-8 terminal display-width utilities (npm `is-fullwidth-code-point` v5 parity).

## Import

```cp
import {
    isFullwidthCodePoint,
    isZeroWidthCodePoint,
    codePointWidth,
    countFullwidthChars,
    hasFullwidthChar,
    stringDisplayWidth,
    truncateToDisplayWidth,
} from "is-fullwidth-code-point";
```

## API Signatures

```cp
export fn isFullwidthCodePoint(cp: u32): bool;
export fn isZeroWidthCodePoint(cp: u32): bool;
export fn codePointWidth(cp: u32): usize;
export fn countFullwidthChars(s: string): usize;
export fn hasFullwidthChar(s: string): bool;
export fn stringDisplayWidth(s: string): usize;
export fn truncateToDisplayWidth(s: string, max_width: usize): string;
```

## Minimal Example

```cp
import { isFullwidthCodePoint, stringDisplayWidth, truncateToDisplayWidth } from "is-fullwidth-code-point";

fn demo(): void {
    const fw = isFullwidthCodePoint(0x4E2D);        // true ('中')
    const w = stringDisplayWidth("hi中");            // 4
    const cut = truncateToDisplayWidth("hi中", 3);   // "hi"
}
```

## Caveats & Borrowing Rules

- **Scalar input**: `isFullwidthCodePoint`, `isZeroWidthCodePoint`, and `codePointWidth` take decoded Unicode scalar values (`u32`), not raw UTF-8 bytes.
- **Zero-copy return**: `truncateToDisplayWidth(s, max_width)` returns a sub-slice borrowing `s`; it never splits a multi-byte UTF-8 sequence and never exceeds `max_width`.
- **ANSI escapes**: `stringDisplayWidth` measures raw Unicode code points; strip ANSI escape sequences first if measuring styled terminal text.
- **Return range**: `codePointWidth(cp)` always returns `0`, `1`, or `2`.
