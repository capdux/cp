# dotenv-expand — AI Agent Quick Ref

Zero-heap .env parser and POSIX variable expansion engine (${VAR}, ${VAR:-default}, $VAR, escaped \$) (clean-room port of npm dotenv + dotenv-expand) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    EnvEntry,
    EnvMap,
    deIsVarChar,
    parseDotenv,
    expandEnvValue,
} from "dotenv-expand";
```

## Core Types & Constants

```cp
class EnvEntry { key: string, value: string }
class EnvMap { entries: [32]EnvEntry, count: usize } // methods: new(...) +4
```

## API Signatures

```cp
fn deIsVarChar(b: u8): bool;
fn parseDotenv(src: string): EnvMap;
fn expandEnvValue(template: string, env: EnvMap, out: []u8): usize;
```

## Minimal Usage

```cp
import { parseDotenv } from "dotenv-expand";

pub fn main() {
    let result = parseDotenv("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"dotenv-expand"`; install via `cap add dotenv-expand`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
