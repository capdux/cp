# toml_writer — AI Agent Quick Ref

Zero-heap TOML 1.0 serializer and formatter (bare/quoted keys, escaped strings, tables, array-of-tables, inline arrays) inspired by Rust toml_writer (`v1.0.0`, `MIT`)

## Import

```cp
import { TomlWriter, isBareKey } from "toml_writer";
```

## Core Types & Constants

```cp
class TomlWriter { out: []u8, pos: usize, overflow: bool } // methods: init(...) +14
```

## API Signatures

```cp
fn isBareKey(k: string): bool;
```

## Minimal Usage

```cp
import { isBareKey } from "toml_writer";

pub fn main() {
    let result = isBareKey("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"toml_writer"`; install via `cap add toml_writer`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add toml_writer
cpc check src/main.cp
```
