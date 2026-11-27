# serialize-javascript — AI Agent Quick Ref

Zero-heap XSS-safe JSON/JavaScript literal serializer escaping HTML script breakouts and line separators (`v6.0.2`, `BSD-3-Clause`)

## Import

```cp
import {
    sjsWriteStr,
    sjsHex,
    escapeUnsafeJson,
    serializeString,
    hasUnsafeScriptChars,
} from "serialize-javascript";
```

## API Signatures

```cp
fn sjsWriteStr(s: string, out: []u8, pos: usize): usize;
fn sjsHex(n: u8): u8;
fn escapeUnsafeJson(json_text: string, out: []u8): usize;
fn serializeString(s: string, out: []u8): usize;
fn hasUnsafeScriptChars(s: string): bool;
```

## Minimal Usage

```cp
import { sjsWriteStr } from "serialize-javascript";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = sjsWriteStr("example", out_buf[0..256], 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"serialize-javascript"`; install via `cap add serialize-javascript`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add serialize-javascript
cpc check src/main.cp
```
