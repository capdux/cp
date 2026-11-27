# character-entities — AI Agent Quick Ref

Zero-allocation HTML5 named and numeric character reference decoder and encoder (covers decode-named-character-reference, parse-entities, stringify-entities) (`v2.0.2`, `MIT`)

## Import

```cp
import {
    mapInvalidCharRef,
    decodeNamedEntity,
    decodeNumericEntity,
    decodeEntities,
    encodeEntities,
} from "character-entities";
```

## API Signatures

```cp
fn mapInvalidCharRef(cp: u32): u32;
fn decodeNamedEntity(name: string): u32?;
fn decodeNumericEntity(body: string): u32?;
fn decodeEntities(input: string, out: []u8): usize;
fn encodeEntities(input: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { decodeNamedEntity } from "character-entities";

pub fn main() {
    let result = decodeNamedEntity("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"character-entities"`; install via `cap add character-entities`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add character-entities
cpc check src/main.cp
```
