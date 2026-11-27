# y18n — AI Agent Quick Ref

Zero-allocation internationalization (i18n) catalog and printf-style formatter (y18n parity) in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import { Y18nEntry, Y18nCatalog, y18WriteByte, formatPrintf } from "y18n";
```

## Core Types & Constants

```cp
class Y18nEntry { key: string, singular: string, plural: string, has_plural: bool }
class Y18nCatalog { locale: string, count: usize, entries: [64]Y18nEntry } // methods: new(...) +9
```

## API Signatures

```cp
fn y18WriteByte(out: []u8, pos: usize, b: u8): usize;
fn formatPrintf(tmpl: string, count: i64, has_count: bool, args: &[]string, out: []u8): usize;
```

## Minimal Usage

```cp
import { formatPrintf } from "y18n";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatPrintf("example", 0, false, /* args: &[]string */, out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"y18n"`; install via `cap add y18n`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
