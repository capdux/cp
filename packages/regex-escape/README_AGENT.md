# regex-escape — AI Agent Quick Ref

Zero-heap regular expression literal escaper, character-class escaper, unescaper, and metacharacter detector. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    isMetaChar,
    isEscapeTarget,
    needsEscape,
    escapedLen,
    escapeRegex,
    escapeCharClass,
    unescapeRegexLiteral,
} from "regex-escape";
```

## API Signatures

```cp
fn isMetaChar(c: u8): bool;
fn isEscapeTarget(c: u8): bool;
fn needsEscape(input: string): bool;
fn escapedLen(input: string): usize;
fn escapeRegex(input: string, out: []u8): usize;
fn escapeCharClass(input: string, out: []u8): usize;
fn unescapeRegexLiteral(input: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { needsEscape } from "regex-escape";

pub fn main() {
    let result = needsEscape("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"regex-escape"`; install via `cap add regex-escape`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
