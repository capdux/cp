# camelcase-keys — AI Agent Quick Ref

Zero-heap streaming JSON object key case converter (camelCase, snake_case, PascalCase) composing camelcase in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    ckIsSpace,
    ckWriteByte,
    ckWriteSlice,
    ckConvertJsonKeys,
    convertJsonKeysToCamel,
    convertJsonKeysToSnake,
    convertJsonKeysToPascal,
} from "camelcase-keys";
```

## API Signatures

```cp
fn ckIsSpace(ch: u8): bool;
fn ckWriteByte(out: []u8, pos: usize, b: u8): usize;
fn ckWriteSlice(out: []u8, pos: usize, src: &[]u8): usize;
fn ckConvertJsonKeys(json: string, mode: u8, out: []u8): usize;
fn convertJsonKeysToCamel(json: string, out: []u8): usize;
fn convertJsonKeysToSnake(json: string, out: []u8): usize;
fn convertJsonKeysToPascal(json: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { ckConvertJsonKeys } from "camelcase-keys";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = ckConvertJsonKeys("example", 0, out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"camelcase-keys"`; install via `cap add camelcase-keys`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
