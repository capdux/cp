# jsonrepair — AI Agent Quick Ref

Zero-allocation malformed & LLM-generated JSON repair engine (jsonrepair / dirty-json / Rust llm_json parity: strips markdown fences, fixes single quotes, unquoted keys, trailing commas, Python True/False/None & truncated brackets) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    isIdentStart,
    isIdentChar,
    appendByte,
    stripMarkdownFence,
    skipWsAndComments,
    repairJson,
} from "jsonrepair";
```

## API Signatures

```cp
fn isIdentStart(c: u8): bool;
fn isIdentChar(c: u8): bool;
fn appendByte(out: []u8, pos: usize, c: u8): usize;
fn stripMarkdownFence(raw: string): string;
fn skipWsAndComments(s: string, start: usize): usize;
fn repairJson(raw: string, out: []u8): string;
```

## Minimal Usage

```cp
import { stripMarkdownFence } from "jsonrepair";

pub fn main() {
    let result = stripMarkdownFence("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"jsonrepair"`; install via `cap add jsonrepair`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add jsonrepair
cpc check src/main.cp
```
