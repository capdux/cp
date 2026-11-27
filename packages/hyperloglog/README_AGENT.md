# hyperloglog — AI Agent Quick Ref

Zero-allocation 256-byte HyperLogLog cardinality estimator with LinearCounting small-range correction and distributed merge (Rust hyperloglog / Redis PFADD/PFCOUNT/PFMERGE parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import { HyperLogLog256, hashBytes64, hashU64, clz56 } from "hyperloglog";
```

## Core Types & Constants

```cp
class HyperLogLog256 { regs: [HLL_M]u8 } // methods: init(...) +8
```

## API Signatures

```cp
fn hashBytes64(data: string): u64;
fn hashU64(val: u64): u64;
fn clz56(w: u64): u8;
```

## Minimal Usage

```cp
import { hashBytes64 } from "hyperloglog";

pub fn main() {
    let result = hashBytes64("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"hyperloglog"`; install via `cap add hyperloglog`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
