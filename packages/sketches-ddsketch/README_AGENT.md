# sketches-ddsketch — AI Agent Quick Ref

Zero-heap DDSketch relative-accuracy quantile sketch for low-memory latency statistics (`v0.3.0`, `Apache-2.0`)

## Import

```cp
import { DDSketch, ddskZeroBuckets, ddskClampAlpha, ddskAddKeyCount } from "sketches-ddsketch";
```

## Core Types & Constants

```cp
class DDSketch { buckets: [DDSK_BUCKETS]u64, alpha: f64, gamma: f64, log_gamma: f64, /* +7 fields */ } // methods: new(...) +7
```

## API Signatures

```cp
fn ddskZeroBuckets(): [DDSK_BUCKETS]u64;
fn ddskClampAlpha(alpha: f64): f64;
fn ddskAddKeyCount(sk: &mut DDSketch, key: i32, cnt: u64);
```

## Minimal Usage

```cp
import { ddskZeroBuckets } from "sketches-ddsketch";

pub fn main() {
    let result = ddskZeroBuckets();
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"sketches-ddsketch"`; install via `cap add sketches-ddsketch`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
