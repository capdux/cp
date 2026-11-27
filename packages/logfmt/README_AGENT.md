# logfmt — AI Agent Quick Ref

Zero-allocation logfmt structured key=value log parser and writer (Heroku/Grafana Loki/Kubernetes) in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import { LogfmtField, LogfmtRecord, LogfmtWriter, needsQuotes } from "logfmt";
```

## Core Types & Constants

```cp
class LogfmtField { key_start: usize, key_len: usize, val_start: usize, val_len: usize }
class LogfmtRecord { key_buf: [512]u8, key_used: usize, val_buf: [1024]u8, val_used: usize, /* +2 fields */ } // methods: parse(...) +3
class LogfmtWriter { buf: []u8, pos: usize, count: usize } // methods: init(...) +5
```

## API Signatures

```cp
fn needsQuotes(val: &[]u8): bool;
```

## Minimal Usage

```cp
import { needsQuotes } from "logfmt";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = needsQuotes(out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"logfmt"`; install via `cap add logfmt`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
