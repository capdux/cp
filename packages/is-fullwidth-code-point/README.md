[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# is-fullwidth-code-point

Zero-allocation UAX #11 Unicode fullwidth code point checker and UTF-8 terminal display-width utilities in pure CP.

- **Upstream Alignment**: `npm` `is-fullwidth-code-point` `v5` (MIT, sindresorhus/is-fullwidth-code-point)
- **Package Version**: `1.0.0`
- **Memory Model**: Zero heap allocation — pure Unicode range checks and zero-copy UTF-8 slicing.

## Installation & Import

```bash
cap add is-fullwidth-code-point
```

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

## Core API

| Symbol | Signature | Description |
| --- | --- | --- |
| `isFullwidthCodePoint` | `fn isFullwidthCodePoint(cp: u32): bool` | Returns `true` if Unicode scalar `cp` is East Asian Wide / Fullwidth (2 terminal columns) per UAX #11. |
| `isZeroWidthCodePoint` | `fn isZeroWidthCodePoint(cp: u32): bool` | Returns `true` if `cp` is a control, combining mark, or zero-width joiner/format character (0 columns). |
| `codePointWidth` | `fn codePointWidth(cp: u32): usize` | Returns terminal cell width of `cp`: `0`, `1`, or `2`. |
| `countFullwidthChars` | `fn countFullwidthChars(s: string): usize` | Counts the number of fullwidth code points in UTF-8 string `s`. |
| `hasFullwidthChar` | `fn hasFullwidthChar(s: string): bool` | Returns `true` if `s` contains at least one fullwidth code point. |
| `stringDisplayWidth` | `fn stringDisplayWidth(s: string): usize` | Computes total terminal display column width of UTF-8 string `s`. |
| `truncateToDisplayWidth` | `fn truncateToDisplayWidth(s: string, max_width: usize): string` | Returns the longest UTF-8 prefix slice of `s` whose display width is `<= max_width` without splitting multi-byte characters. |

## Examples

### 1. Code Point Width Classification

```cp
import { isFullwidthCodePoint, codePointWidth } from "is-fullwidth-code-point";

fn checkChars(): void {
    const is_cjk = isFullwidthCodePoint(0x4E2D); // true ('中')
    const is_ascii = isFullwidthCodePoint(0x41); // false ('A')
    const w = codePointWidth(0x1F600);           // 2 (emoji)
}
```

### 2. UTF-8 Display Width & Safe Prefix Truncation

```cp
import { stringDisplayWidth, truncateToDisplayWidth, hasFullwidthChar } from "is-fullwidth-code-point";

fn layoutDemo(): void {
    const label = "CP语言2026";
    const width = stringDisplayWidth(label);           // 10 (2 + 2*2 + 4)
    const has_fw = hasFullwidthChar(label);            // true
    const clipped = truncateToDisplayWidth(label, 5);  // "CP语" (width 4, never splits '言')
}
```

## Differences from Upstream & Highlights

- **Built-in UTF-8 String Utilities**: Beyond single-code-point `isFullwidthCodePoint`, adds `isZeroWidthCodePoint`, `codePointWidth`, `stringDisplayWidth`, and zero-copy `truncateToDisplayWidth` for CLI table/progress-bar rendering.
- **Zero-Copy Safe Truncation**: `truncateToDisplayWidth` slices `s` strictly on UTF-8 code point boundaries with zero heap allocation.
