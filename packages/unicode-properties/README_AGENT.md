# unicode-properties — AI Agent Quick Ref

Zero-allocation Unicode character properties, UAX #31 identifier validation (XID_Start/XID_Continue), White_Space, Default_Ignorable, and UTS #51 Emoji classification in pure CP. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    decodeUtf8Strict,
    isXidStart,
    isXidContinue,
    isUnicodeWhitespace,
    isEmojiPresentation,
    isEmoji,
    isDefaultIgnorable,
    isValidIdentifier,
} from "unicode-properties";
```

## API Signatures

```cp
fn decodeUtf8Strict(s: string, pos: usize, next_pos: &mut usize): u32;
fn isXidStart(cp: u32): bool;
fn isXidContinue(cp: u32): bool;
fn isUnicodeWhitespace(cp: u32): bool;
fn isEmojiPresentation(cp: u32): bool;
fn isEmoji(cp: u32): bool;
fn isDefaultIgnorable(cp: u32): bool;
fn isValidIdentifier(s: string): bool;
```

## Minimal Usage

```cp
import { decodeUtf8Strict } from "unicode-properties";

pub fn main() {
    let result = decodeUtf8Strict("example", 0, /* next_pos: &mut usize */);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"unicode-properties"`; install via `cap add unicode-properties`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
